#pragma once
#ifdef TRANSPORT_BLE

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "Transport.h"
#include "config.h"

// ---------------------------------------------------------------------------
// BLE-MIDI 1.0 — le transport de production, celui du téléphone.
//
// L'argument décisif face à un JSON sur WebSocket : la trame BLE-MIDI porte
// NATIVEMENT un horodatage 13 bits en millisecondes. On horodate à la source
// (instant de la frappe, cf. PadScanner::poll) et le récepteur reconstruit le
// timing exact — le jitter de 10-20 ms du BLE disparaît complètement à
// l'enregistrement. C'est ce qui sauve l'usage « enregistrer un morceau ».
//
// Format de trame (spec BLE-MIDI 1.0) :
//
//   [0] header    = 0b10 | ts[12:7]      -> 0x80 | ((ms >> 7) & 0x3F)
//   [1] timestamp = 0b1  | ts[6:0]       -> 0x80 |  (ms       & 0x7F)
//   [2..] message MIDI brut
//
// Le compteur de millisecondes est sur 13 bits : il boucle toutes les 8192 ms.
// Le récepteur doit dérouler ces boucles (voir web/index.html).
// ---------------------------------------------------------------------------

#define MIDI_SERVICE_UUID "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define MIDI_CHAR_UUID    "7772E5DB-3868-4112-A1A9-F2669D106BF3"

class BleMidiTransport : public Transport, public NimBLEServerCallbacks {
 public:
  void begin() override {
    Serial.begin(115200);
    delay(50);

    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    // La spec BLE-MIDI recommande un MTU d'au moins 23 ; on demande plus large
    // pour pouvoir grouper plusieurs messages dans une trame si besoin.
    NimBLEDevice::setMTU(69);

    server_ = NimBLEDevice::createServer();
    server_->setCallbacks(this);

    NimBLEService* svc = server_->createService(MIDI_SERVICE_UUID);
    char_ = svc->createCharacteristic(
        MIDI_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
    svc->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    // Indispensable : Chrome filtre sur cet UUID dans requestDevice().
    adv->addServiceUUID(MIDI_SERVICE_UUID);
    adv->setScanResponse(true);
    NimBLEDevice::startAdvertising();

    Serial.printf("[ble] annonce en cours sous le nom \"%s\"\n", BLE_DEVICE_NAME);
  }

  bool ready() const override { return connected_; }

  void sendMidi(const uint8_t* msg, size_t len, int64_t tUs) override {
    if (!connected_ || !char_ || len > 16) return;

    const uint16_t ms = (uint16_t)((tUs / 1000) & 0x1FFF);  // 13 bits
    uint8_t pkt[18];
    pkt[0] = (uint8_t)(0x80 | ((ms >> 7) & 0x3F));
    pkt[1] = (uint8_t)(0x80 | (ms & 0x7F));
    memcpy(pkt + 2, msg, len);

    char_->setValue(pkt, 2 + len);
    char_->notify();
  }

  const char* name() const override { return "ble-midi"; }

  // --- NimBLEServerCallbacks ------------------------------------------------
  void onConnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
    connected_ = true;
    // 6 × 1,25 ms = 7,5 ms, le minimum autorisé par la spec BLE. Sans ça
    // l'intervalle négocié par défaut (souvent 30-50 ms) plombe la latence.
    // Le central peut refuser : c'est une requête, pas un ordre.
    s->updateConnParams(desc->conn_handle, 6, 12, 0, 200);
    Serial.println("[ble] connecte");
  }

  void onDisconnect(NimBLEServer* s) override {
    (void)s;
    connected_ = false;
    Serial.println("[ble] deconnecte, reprise de l'annonce");
    NimBLEDevice::startAdvertising();
  }

 private:
  NimBLEServer* server_ = nullptr;
  NimBLECharacteristic* char_ = nullptr;
  volatile bool connected_ = false;
};

#endif  // TRANSPORT_BLE
