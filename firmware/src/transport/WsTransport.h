#pragma once
#ifdef TRANSPORT_WS

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "Transport.h"
#include "config.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copie firmware/include/secrets.example.h en secrets.h et remplis le WiFi."
#endif

// ---------------------------------------------------------------------------
// Transport de dev sur le LAN. Trame binaire, 4 + n octets :
//
//   [0..3] timestamp µs depuis le boot, uint32 little-endian
//   [4..n] octets MIDI bruts
//
// ⚠️ Une page en https:// ne peut PAS ouvrir un ws:// (contenu mixte bloqué).
// Ce transport se teste donc depuis une page servie en http:// — typiquement le
// laptop. Le téléphone en https://, c'est le transport BLE.
//
// ⚠️ Sur ESP32 classique, WiFi et BLE partagent la radio : un seul des deux
// environnements à la fois, jamais les deux.
// ---------------------------------------------------------------------------

class WsTransport : public Transport {
 public:
  void begin() override {
    Serial.begin(115200);
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("[ws] connexion a %s", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      Serial.print('.');
    }
    Serial.printf("\n[ws] pret : ws://%s:%u\n", WiFi.localIP().toString().c_str(), WS_PORT);
    instance = this;
    server_.onEvent(onEvent);
    server_.begin();
  }

  void loop() override { server_.loop(); }

  bool ready() const override { return clients_ > 0; }

  void sendMidi(const uint8_t* msg, size_t len, int64_t tUs) override {
    if (len > 8) return;
    uint8_t frame[12];
    const uint32_t t = (uint32_t)tUs;
    frame[0] = (uint8_t)(t & 0xFF);
    frame[1] = (uint8_t)((t >> 8) & 0xFF);
    frame[2] = (uint8_t)((t >> 16) & 0xFF);
    frame[3] = (uint8_t)((t >> 24) & 0xFF);
    memcpy(frame + 4, msg, len);
    server_.broadcastBIN(frame, 4 + len);
  }

  const char* name() const override { return "websocket"; }

  // WebSocketsServer veut un callback libre : on passe par un singleton.
  static WsTransport* instance;

 private:
  static void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    (void)num;
    (void)payload;
    (void)len;
    if (!instance) return;
    if (type == WStype_CONNECTED) instance->clients_++;
    if (type == WStype_DISCONNECTED && instance->clients_ > 0) instance->clients_--;
  }

  WebSocketsServer server_{WS_PORT};
  uint8_t clients_ = 0;
};

#endif  // TRANSPORT_WS
