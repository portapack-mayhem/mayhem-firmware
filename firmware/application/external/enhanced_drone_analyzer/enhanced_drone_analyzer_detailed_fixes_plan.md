# DETAL'NYJ PLAN ISPRAVLENIJ DLYA ENHANCED DRONE ANALYZER

## KRITICHESKIE OSHIBKI I OSPRANENIYA

### 🔥 KRITICHESKAJA OSHIBKA: OTSUTSTVUJUSHHAJA LOGIKA SKANIROVANIYA
**TEKUHSHAJA VERSIJA:** Potok skanirovanija prosto zasypaet bez vypolnenija raboty
**STARYJ RUKOPISNYJ KOD:** Polnofunkcional'nyj potok so mnogimi rezhimami skanirovanija
- **RESHENIE:** Perenesti `perform_scan_cycle()`, `scanning_thread()`, i vsju logiku cikla iz staryh fajlov
- **SRVAVNENIE:** Sleduet tipu Looking Glass (potokovoe upravlenie) + Detector RX (ciklicheskoe skanirovanie)

### 🔥 KRITICHESKAJA OSHIBKA: DYMMY RSSI ZNACHENIYA
**TEKUHSHAJA VERSIJA:** Vozvrashhaet 0 dlya RSSI, bez real'nogo izmerenija
**STARYJ KOD:** Polnyj obrabotchik spektra s raschetami i fil'traciej
- **RESHENIE:** Perenesti `handle_channel_spectrum()` i `process_channel_spectrum_data()`

### 🔥 NEDOSTAJUSHHAJA VALIDACIYA SIGNALOV
**PROBLEMA:** Otsutstvujut hysteresis, ring buffer, porogovye algoritmy
**RESHENIE:** Perenesti `DetectionRingBuffer` i `process_rssi_detection_with_hysteresis`

## DETAL'NOE SRVAVNENIE S DRUGIMI PRILOZhENIJAMI

### HARDWARE INITIALIZACIYA: SOVPADENIE PATTERNOV
| Pattern | Looking Glass | EDA (staryj) | Status |
|---------|---------------|-------------|--------|
| Posledovatel'nost' | baseband → enable → tune ✅ | baseband → enable → tune ✅ | SOVPADAET |
| set_squelch_level(0) | ✅ | ✅ | SOVPADAET |
| spectrum_streaming_start() | ✅ | ✅ | SOVPADAET |

### SPECTRUM PROCESSING: OPTIMAL'NAJA SLOZHNOST'
Looking Glass: Kompleksnyj obrabotchik s DC spike i bin mapping
EDA: Prostoj peak/average + smoothing - bolee prigoden dlja embedded

**RESHENIE:** Ispol'zovat' uproshhennyj variant iz staryh fajlov (effektivnost' bez peregruzki)

### DETECTION ALGORITHMY: GIBRIDNYJ PODHOD NUZHEN
- Detector RX: Timer-based + freq hopping → OCHEN' PROSTO
- Recon App: Frequency stepping + validation → POSLEDOVATEL'NYJ
- EDA: Ring buffer hysteresis + threat levels → OPTIMAL'NYJ DLJA DRONOV

## FAZOVYJ PLAN ISPRAVLENIJ

### FAZA 1: KORENNYE SISTEMNYE KOMPONENTY (3-5 CHASOV)
```cpp
// Iz ui_drone_scanner.cpp staryj:
void DroneScanner::perform_scan_cycle(DroneHardwareController& hardware)
// Iz ui_drone_hardware.cpp staryj:
void DroneHardwareController::handle_channel_spectrum(const ChannelSpectrum& spectrum)
// Iz ui_drone_detection_ring.cpp staryj:
class DetectionRingBuffer
// Iz ui_drone_scanning_coordinator.cpp staryj:
class ScanningCoordinator::coordinated_scanning_thread()
```

### FAZA 2: DETEKCIYA I ALGORITMY (2-3 CHASA)
```cpp
// Iz ui_drone_scanner.cpp staryj:
void DroneScanner::process_rssi_detection(const freqman_entry& entry, int32_t rssi)
bool DroneScanner::validate_detection_simple(int32_t rssi_db, ThreatLevel threat)
// Ring buffer hysteresis logic
```

### FAZA 3: WIDEBAND SKANIROVANIE (1-2 CHASA)
```cpp
// Iz ui_drone_scanner.cpp staryj:
void DroneScanner::perform_wideband_scan_cycle(DroneHardwareController& hardware)
void DroneScanner::process_wideband_slice_samples()
// Slice-based frequency scanning
```

### FAZA 4: BAZY DANNYH I LOGIROVANIE (1-2 CHASA)
```cpp
// Iz ui_drone_database.cpp staryj:
class DroneFrequencyDatabase
// Iz ui_detection_logger.cpp staryj:
class DroneDetectionLogger::log_detection()
// CSV export system
```

### FAZA 5: UI INTEGRACIYA I NASTROJKI (1-2 CHASA)
```cpp
// Iz ui_drone_settings_complete.hpp staryj:
struct DroneAnalyzerSettings
// Soedinenie s suhestvujushhimi UI komponentami
```

## DETAL'NYE TEHNICHESKIE TREBOVANIYA

### MEMORY MANAGEMENT
- **Ring Buffer:** 75% optimisation vs static arrays
- **Fixed Arrays:** MAX_TRACKED_DRONES = 8 (ne menjat')
- **No Dynamic Allocation:** Vse ChibiOS heap-free

### PERFORMANCE CONSTRAINTS
- **Thread Stack:** 2048 bytes (uzhe ustanovleno)
- **Message Handling:** Async, bez blokirovok UI
- **Spectrum Processing:** < 50ms per cycle

### PORTAPACK COMPATIBILITY
- **Baseband API:** Wideband spectrum tag
- **Message System:** ChannelSpectrum + FrameSync
- **Threading:** ChibiOS patterns tol'ko

## RISC ANALIZ I MITIGACIYA

### RISC 1: MEMORY OVERFLOW
**SYMPTOMY:** Ring buffer nekontroliruemoe uvelichenie
**MITIGACIYA:** MAX_TRACKED_DRONES limit + overflow protection

### RISC 2: THREAD RACE CONDITIONS
**SYMPTOMY:** Pohozhe na "knownConditionTrueFalse" v cppcheck
**MITIGACIYA:** Mutex protection + atomic operations

### RISC 3: SPECTRUM PROCESSING HANG
**SYMPTOMY:** Streaming blokiruet UI thread
**MITIGACIYA:** Async message handling + timeout logic

## TESTING I VALIDACIYA

### UNIT TESTING (posle implementacii)
```cpp
// ScanningCoordinator functionality
bool is_scanning_active()
// Spectrum data processing
int32_t get_real_rssi_from_hardware()
// Detection algorithms
bool validate_detection_simple()
// Ring buffer operations
uint8_t get_detection_count(size_t freq_hash)
```

### INTEGRATION TESTING
- Hardware initialization sequence
- Spectrum streaming workflow
- Detection → Logging chain
- UI update timing

### PERFORMANCE TESTING
- Thread stack usage
- Message processing latency
- Memory consumption limits

## ZAVERSHENIE I OZHIDAEMYE REZULTATY

### POSLE IMPLEMENTACII
- ✅ **Rabochaja funkcional'nost':** App dejstvitel'no skaniruet i detektiruet
- ✅ **Pravil'nyj RSSI:** Real'nye znachenija iz spektral'nyh dannyh
- ✅ **Stabil'naja detekcija:** Hysteresis predotvrashhaet lzhnye srabatyvanija
- ✅ **Multi-mode:** Database, Wideband, Hybrid rezhimy

### TEHNICHESKIE HARAKTERISTIKI
- **Vremja implementacii:** 8-12 chasov obshhego truda
- **Roki:** 1-2 nedeli s testing
- **Narushenie backwards compatibility:** Minimal'noe (tol'ko vnutrennie API)
- **Tesnost' svjazej:** Novye komponenty sohranenie modularnoj arhitektury

## STATUS I SLEDUJUSHHIE SHAGI

**Last Reading/Correction Error Status**: Analysis completed successfully. Old working code identified for transfer. Implementation plan detailed with risk mitigation.

**TODO SUMMARY:** Ready for systematic code transfer surgery from garbage version to restore functionality.

---

### PRIORITIZIROVANNYJ SPISOK NEPOSREDSTVENNYH DEJSTVIJ

1. **NEMEDLENNO:** Kopirovat' i integrirovat' `perform_scan_cycle()` i `scanning_thread()`
2. **V TEKUHHEM SSORE:** Zamenit' dummy RSSI na real'nye spectrum calculations
3. **DOPOLNITEL'NO:** Dobavit' DetectionRingBuffer dlja hysteresis
4. **VALIDACIJA:** Proverit' sohranenie Portapack patterns
5. **TESTING:** Zapustit' basic scanning bez UI hangsulovin
