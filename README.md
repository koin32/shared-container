# varser-project

## 📖 Description (EN)

`varser-project` is a prototype system for data sharing between multiple user-space processes via shared containers in a Linux kernel module.  

### Features

- **Containers**
  - Unique by name.
  - Store named variables.
  - Automatically removed when unused.



- **Variables**
  - Supported types: `int32`, `int64`, `uint8`, `uint64`, `float`, `double`, `string`, `blob`.
  - Strings and blobs support `size:` constraints.
  - Default values supported.



- **Synchronization (Lock Policy)**
  - Configured in YAML (`lock_policy`).
  - Modes:
    - `none` — no locks.
    - `per_container_mutex` — one mutex per container.
    - `per_variable_rw` — read/write lock per variable.

- **Multi-process**
  - Multiple apps can share a container.
  - All see consistent values.



- **Kernel ↔ User interface**
  - Through `/dev/varser`.
  - Controlled via `ioctl`.
  - User library `libvarser.a` provides C++20 API.

---

## 🔧 Requirements

- Linux with kernel module support  
- `linux-headers-$(uname -r)`  
- GCC or Clang  
- CMake ≥ 3.16  
- C++20  
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (`sudo apt install libyaml-cpp-dev`)  

---

## ⚙️ Build & Install

### 1. Kernel module

```bash
cd kernel
make
```

Result: `varser.ko`

Load:

```bash
sudo insmod varser.ko
dmesg | tail -n 20
```

Unload:

```bash
sudo rmmod varser
```

Device: `/dev/varser`

---

### 2. User library & demo

```bash
cd user
mkdir build && cd build
cmake ..
make
```

Result:

* `libvarser.a` — library
* `varser_demo` — demo app

---

## 📖 Synchronization

`varser-project` supports multiple **lock policies** to ensure safe concurrent access when several processes read and write variables in the same container.

### Lock policies

* **`none`**

  * No locking at all.
  * Fastest option.
  * Suitable only when:
    * A single process works with the container, or
    * All processes only *read* variables, never modify them.
  * ⚠️ Writing from multiple processes can cause race conditions.
* **`per_container_mutex`**
  * A single **mutex** is associated with the whole container.
  * Any read or write operation on any variable takes this lock.
  * Guarantees consistency but reduces concurrency:
    * Only one process can access the container at a time.
  * Recommended when:
    * Operations are complex and must be serialized.
    * Variables are logically interdependent (must be updated atomically together).

* **`per_variable_rw`**

  * Each variable has its own **read/write lock**:
    * Multiple readers can access the same variable in parallel.
    * Writers get exclusive access to that variable.
  * Other variables remain accessible concurrently.
  * Recommended for:
    * High concurrency scenarios.
    * Containers with many independent variables.
  * Provides the best trade-off between safety and performance.

### Examples of race conditions

* **Without locks**: two processes write to `counter` simultaneously → final value is unpredictable.
* **Per-container lock**: one process writes to `counter`, another reads `temperature` → second process is blocked until first finishes.
* **Per-variable rwlock**: one process writes `counter`, another reads `temperature` → no conflict, both succeed concurrently.

---

## 🚀 Usage

### YAML definition

```yaml
container: varser_foo
lock_policy: "per_variable_rw"
variables:
  - name: counter
    type: int64
    default: 0
  - name: flag
    type: uint8
    default: 1
  - name: temperature
    type: double
    default: 20.5
  - name: note
    type: string
    size: 256
    default: "hello"
  - name: blob_data
    type: blob
    size: 256
```

### C++ example

```cpp
#include "varser/varser.hpp"
#include <iostream>
using namespace varser;

int main() {
container.yaml.example
    auto c = ContainerManager::instance().load_from_yaml("../examples/example.yaml");
    if (!c || !c->open()) {
        std::cerr << "Failed to open container\n";
        return 1;
    }

    int64_t counter = 0;
    c->get<int64_t>("counter", counter);
    std::cout << "counter initial = " << counter << "\n";

    c->set<int64_t>("counter", 42);
    c->get<int64_t>("counter", counter);
    std::cout << "counter now = " << counter << "\n";

    double temp = 0.0;
    c->get<double>("temperature", temp);
    std::cout << "temperature = " << temp << "\n";

    c->close();
    return 0;
}
```

---

## 📂 Project structure

```
varser-project/
├─ kernel/          # kernel module
├─ user/            # library and demo
├─ examples/        # YAML examples
├─ container.yaml   # default YAML
└─ README_EN.md
```

---

## ❓ FAQ

**Q:** `yaml-cpp not found`
**A:** Install: `sudo apt install libyaml-cpp-dev`
**Q:** `/dev/varser` missing
**A:** Load module: `sudo insmod kernel/varser.ko`
**Q:** Can multiple apps share one container?
**A:** Yes. The container exists as long as at least one app keeps it open.

---

## 📖 Описание (RU)

`varser-project` — это прототип системы обмена данными между несколькими пользовательскими процессами через общий контейнер через модуль ядра Linux.  

### Возможности
- **Контейнеры**
  - Уникальны по имени.
  - Хранят набор именованных переменных.
  - Автоматически удаляются, если их никто не использует.

- **Переменные**
  - Поддерживаемые типы: `int32`, `int64`, `uint8`, `uint64`, `float`, `double`, `string`, `blob`.
  - Строки и блобы поддерживают ограничение размера (`size:` в YAML).
  - Поддержка значений по умолчанию.

- **Синхронизация (Lock Policy)**
  - Настраивается в YAML (`lock_policy`).
  - Режимы:
    - `none` — без блокировок.
    - `per_container_mutex` — мьютекс на контейнер.
    - `per_variable_rw` — блокировка чтения/записи на переменную.

- **Мультипроцессная работа**
  - Несколько процессов могут работать с одним контейнером.
  - Все видят актуальные данные.

- **Интерфейс ядра и пользователя**
  - Через символическое устройство `/dev/varser`.
  - Управление через `ioctl`.
  - Пользовательская библиотека `libvarser.a` даёт удобный API на C++20.

---

## 🔧 Требования

- GCC или Clang  
- CMake ≥ 3.16  
- C++20  
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (`sudo apt install libyaml-cpp-dev`)  

---

## ⚙️ Сборка и установка

### 1. Модуль ядра
```bash
cd kernel
make
```

Результат: `varser.ko`

Загрузка:

```bash
sudo insmod varser.ko
dmesg | tail -n 20
```

Удаление:

```bash
sudo rmmod varser
```

После загрузки доступно устройство `/dev/varser`.

---

### 2. Библиотека и пример

```bash
cd user
mkdir build && cd build
cmake ..
make
```

Результат:

* `libvarser.a` — библиотека
* `varser_demo` — пример

---

## 📖 Синхронизация 

`varser-project` поддерживает разные **политики блокировок**, чтобы безопасно работать с данными, если несколько процессов одновременно читают и пишут переменные в одном контейнере.

### Политики блокировок

* **`none`**
  * Блокировки отсутствуют.
  * Максимальная скорость.
  * Подходит только если:
    * С контейнером работает один процесс, или
    * Все процессы только читают переменные.
  * ⚠️ Запись из нескольких процессов приведёт к гонкам и непредсказуемым значениям.

* **`per_container_mutex`**
  * Один общий **мьютекс** на весь контейнер.
  * Любая операция (чтение или запись) блокирует контейнер целиком.
  * Обеспечивает полную консистентность, но снижает параллелизм:
    * В каждый момент времени контейнером может пользоваться только один процесс.
  * Рекомендуется, если:
    * Операции сложные и должны выполняться строго по очереди.
    * Переменные взаимосвязаны и должны обновляться атомарно.
* **`per_variable_rw`**
  * Для каждой переменной — свой **rwlock** (блокировка на чтение/запись):
    * Несколько процессов могут параллельно читать одну переменную.
    * Запись всегда эксклюзивная.
  * Остальные переменные доступны одновременно без блокировки.
  * Оптимально, если:
    * Контейнер большой и в нём много независимых переменных.
    * Требуется высокая степень параллелизма.

### Примеры гонок

* **Без блокировок**: два процесса одновременно пишут в `counter` → итоговое значение случайно.
* **Мьютекс на контейнер**: один процесс пишет `counter`, другой читает `temperature` → второй ждёт, пока первый освободит контейнер.
* **RW-блокировка на переменную**: один процесс пишет `counter`, другой читает `temperature` → операции выполняются параллельно без конфликтов.

---

## 🚀 Использование

### YAML описание

```yaml
container: varser_foo
lock_policy: "per_variable_rw"
variables:
  - name: counter
    type: int64
    default: 0
  - name: flag
    type: uint8
    default: 1
  - name: temperature
    type: double
    default: 20.5
  - name: note
    type: string
    size: 256
    default: "hello"
  - name: blob_data
    type: blob
    size: 256
```

### C++ пример

```cpp
#include "varser/varser.hpp"
#include <iostream>
using namespace varser;

int main() {
    auto c = ContainerManager::instance().load_from_yaml("../examples/container.yaml.example");
    if (!c || !c->open()) {
        std::cerr << "Ошибка открытия контейнера\n";
        return 1;
    }

    int64_t counter = 0;
    c->get<int64_t>("counter", counter);
    std::cout << "counter initial = " << counter << "\n";

    c->set<int64_t>("counter", 42);
    c->get<int64_t>("counter", counter);
    std::cout << "counter now = " << counter << "\n";

    double temp = 0.0;
    c->get<double>("temperature", temp);
    std::cout << "temperature = " << temp << "\n";

    c->close();
    return 0;
}
```

---

## 📂 Структура проекта

```
varser-project/
├─ kernel/          # модуль ядра
├─ user/            # библиотека и пример
├─ examples/        # YAML примеры
├─ container.yaml   # YAML по умолчанию
└─ README_RU.md
```

---

## ❓ FAQ

**Q:** `yaml-cpp not found`
**A:** Установите пакет: `sudo apt install libyaml-cpp-dev`
**Q:** Нет `/dev/varser`
**A:** Загрузите модуль: `sudo insmod kernel/varser.ko`
**Q:** Несколько приложений могут работать с одним контейнером?
**A:** Да, контейнер живёт, пока хотя бы одно приложение открыто.
