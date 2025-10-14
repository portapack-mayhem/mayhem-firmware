# Анализ дублирования функций в Enhanced Drone Analyzer

## Обнаруженные случаи дублирования:

### 1. **Дублированные enum определения**

**Файлы:**
- `ui_minimal_drone_analyzer.hpp`: lines 38-62 (DroneType, ThreatLevel)
- `ui_drone_database.hpp`: lines 14-39 (DroneType, ThreatLevel)
- `ui_drone_spectrum_scanner.hpp`: lines 14-28 (ThreatLevel, DroneType)
- `ui_drone_tracking.hpp`: lines 16-21 (MovementTrend)

**Проблема:** Одни и те же enum'ы определены в нескольких местах

**Решение:** Объединить в общий header `ui_drone_types.hpp`

### 2. **Дублированные структуры**

**Структура `TrackedDrone`:**
- `ui_minimal_drone_analyzer.hpp`: lines 91-121 (полная реализация с методами)
- `ui_drone_tracking.hpp`: lines 14-106 (аналогичная со своим конструктором)

**Проблема:** Полностью идентичная структура в двух файлах

**Решение:** Оставить в `ui_drone_tracking.hpp`, удалить из `ui_minimal_drone_analyzer.hpp`

### 3. **Дублированные функциональности классов**

**DroneSpectrumScanner vs DroneScanningManager:**
- `ui_drone_spectrum_scanner.hpp`: Полный класс со scanning methods
- `ui_drone_scanning.hpp`: Дублирующиеся методы и структура

**Проблема:** Две отдельные реализации одной функциональности

## План рефакторинга:

### Фаза 1: Объединить типы
1. Создать `ui_drone_types.hpp`
2. Перенести все enum's указ сюда
3. Обновить includes во всех файлах

### Фаза 2: Убрать дублированные структуры
1. Удалить `TrackedDrone` из `ui_minimal_drone_analyzer.hpp`
2. Оставить только в `ui_drone_tracking.hpp`
3. Обновить includes

### Фаза 3: Объединить scanner классы
1. Выбрать лучший из двух классов
2. Перенести всю функциональность
3. Удалить дублированный класс

### Фаза 4: Тестирование и cleanup
1. Проверить компиляцию
2. Тестировать функциональность
3. Удалить неиспользуемые файлы
