#!/bin/bash
set -e

echo "================================================"
echo "🚀 KContainer Build and Run Script"
echo "================================================"

echo -e "\n\033[1;34m[1] Сборка и загрузка модуля ядра...\033[0m"
cd kernel
echo -e "\033[1;33m🛠️  Очистка и компиляция модуля...\033[0m"
make clean && make
if lsmod | grep -q kcontainer; then
    echo -e "\033[1;33m📤 Выгрузка существующего модуля kcontainer...\033[0m"
    sudo rmmod kcontainer || { echo -e "\033[1;33m⚠️  Не удалось выгрузить модуль (возможно используется)\033[0m"; }
fi

echo -e "\033[1;33m📦 Загрузка нового модуля...\033[0m"
sudo insmod kcontainer.ko || { echo -e "\033[1;31m❌ Ошибка загрузки модуля\033[0m"; exit 1; }

echo -e "\033[1;33m🔧 Настройка прав доступа...\033[0m"
sudo chmod 666 /dev/kcontainer || { echo -e "\033[1;33m⚠️  Не удалось установить права, но продолжим\033[0m"; }

echo -e "\033[1;33m📊 Статус модуля:\033[0m"
lsmod | grep kcontainer || { echo -e "\033[1;31m❌ Модуль не загружен\033[0m"; exit 1; }
echo ""

echo -e "\033[1;34m[2] Сборка пользовательской библиотеки...\033[0m"
cd ../userlib
mkdir -p build
cd build

echo -e "\033[1;33m🔍 Поиск TinyXML2...\033[0m"
TINYXML2_AVAILABLE=0
TINYXML2_DIR=""

# Проверяем различные возможные расположения TinyXML2
POSSIBLE_PATHS=(
    "/usr/lib/x86_64-linux-gnu/cmake/tinyxml2"
    "/usr/local/lib/cmake/tinyxml2"
    "/usr/lib/cmake/tinyxml2"
    "/opt/homebrew/lib/cmake/tinyxml2"  # Для macOS
)

for path in "${POSSIBLE_PATHS[@]}"; do
    if [ -d "$path" ]; then
        TINYXML2_AVAILABLE=1
        TINYXML2_DIR="$path"
        echo -e "\033[1;32m✅ Найден TinyXML2: $path\033[0m"
        break
    fi
done

# Дополнительная проверка через pkg-config
if [ $TINYXML2_AVAILABLE -eq 0 ] && pkg-config --exists tinyxml2; then
    TINYXML2_AVAILABLE=1
    echo -e "\033[1;32m✅ TinyXML2 найден через pkg-config\033[0m"
fi

if [ $TINYXML2_AVAILABLE -eq 1 ]; then
    if [ -n "$TINYXML2_DIR" ]; then
        echo -e "\033[1;33m🔄 Конфигурация с TinyXML2...\033[0m"
        cmake -DTinyXML2_DIR="$TINYXML2_DIR" ..
    else
        cmake ..
    fi
else
    echo -e "\033[1;33m⚠️  TinyXML2 не найден, сборка без XML поддержки\033[0m"
    cmake -DFORCE_NO_XML=ON ..
fi

echo -e "\033[1;33m🔨 Компиляция библиотеки...\033[0m"
make -j$(nproc)
echo -e "\033[1;32m✅ Библиотека собрана: libkcontainer.so\033[0m"
echo ""

echo -e "\033[1;34m[3] Сборка примеров...\033[0m"
cd ../../examples
mkdir -p build
cd build

echo -e "\033[1;33m🔨 Компиляция демо-примеров...\033[0m"
if [ $TINYXML2_AVAILABLE -eq 1 ] && [ -n "$TINYXML2_DIR" ]; then
    echo -e "\033[1;33m📦 Сборка с поддержкой XML\033[0m"
    cmake -DTinyXML2_DIR="$TINYXML2_DIR" ..
else
    echo -e "\033[1;33m📦 Сборка без XML поддержки\033[0m"
    cmake ..
fi

make -j$(nproc)

# Проверяем сборку демо-примеров
echo -e "\033[1;33m📊 Проверка собранных демо:\033[0m"
DEMOS=("demo1" "ipc_writer" "ipc_reader" "demo3" "persistence_demo" "container_monitor")

# XML демо (только если есть поддержка XML)
if [ $TINYXML2_AVAILABLE -eq 1 ]; then
    DEMOS+=("xml_config_demo" "create_from_xml")
fi

for demo in "${DEMOS[@]}"; do
    if [ -f "./$demo" ]; then
        echo -e "\033[1;32m✅ $demo собран\033[0m"
    else
        echo -e "\033[1;33m⚠️  $demo не собран\033[0m"
    fi
done
echo -e "\033[1;32m✅ Примеры собраны\033[0m"
echo ""

echo -e "\033[1;34m[4] Запуск демонстраций...\033[0m"
echo "================================================"

# Создаем конфигурационный XML файл если его нет
if [ ! -f "containers_config.xml" ] && [ -f "../containers_config.xml" ]; then
    cp ../containers_config.xml .
    echo -e "\033[1;33m📋 Конфигурационный файл скопирован\033[0m"
fi

echo -e "\n\033[1;36m=== Демо 1: Базовое использование ===\033[0m"
if [ -f "./demo1" ]; then
    LD_LIBRARY_PATH=../../userlib/build ./demo1
else
    echo -e "\033[1;33m⚠️  demo1 не собран, пропускаем\033[0m"
fi
echo ""

echo -e "\033[1;36m=== Демо 2: Межпроцессное взаимодействие ===\033[0m"
if [ -f "./ipc_writer" ] && [ -f "./ipc_reader" ]; then
    echo -e "\033[1;33m🖊️  Запуск писателя...\033[0m"
    LD_LIBRARY_PATH=../../userlib/build ./ipc_writer &
    WRITER_PID=$!
    sleep 1

    echo -e "\033[1;33m📖 Запуск читателя...\033[0m"
    LD_LIBRARY_PATH=../../userlib/build ./ipc_reader

    wait $WRITER_PID
else
    echo -e "\033[1;33m⚠️  IPC демо не собраны, пропускаем\033[0m"
fi
echo ""

echo -e "\033[1;36m=== Демо 3: Многопоточность ===\033[0m"
if [ -f "./demo3" ]; then
    LD_LIBRARY_PATH=../../userlib/build ./demo3
else
    echo -e "\033[1;33m⚠️  demo3 не собран, пропускаем\033[0m"
fi
echo ""

echo -e "\033[1;36m=== Демо 4: Persistence ===\033[0m"
if [ -f "./persistence_demo" ]; then
    LD_LIBRARY_PATH=../../userlib/build ./persistence_demo
else
    echo -e "\033[1;33m⚠️  Persistence demo не собран, пропускаем\033[0m"
fi
echo ""

echo -e "\033[1;36m=== Демо 5: Мониторинг ===\033[0m"
if [ -f "./container_monitor" ]; then
    LD_LIBRARY_PATH=../../userlib/build ./container_monitor
else
    echo -e "\033[1;33m⚠️  Container monitor не собран, пропускаем\033[0m"
fi
echo ""

# XML демонстрации (только если есть поддержка XML)
if [ $TINYXML2_AVAILABLE -eq 1 ]; then
    echo -e "\033[1;36m=== Демо 6: XML конфигурация ===\033[0m"
    if [ -f "./xml_config_demo" ]; then
        if [ -f "containers_config.xml" ]; then
            LD_LIBRARY_PATH=../../userlib/build ./xml_config_demo
        else
            echo -e "\033[1;33m⚠️  Файл containers_config.xml не найден\033[0m"
            echo -e "\033[1;33m💡 Создайте конфигурационный файл или скопируйте из ../containers_config.xml\033[0m"
        fi
    else
        echo -e "\033[1;33m⚠️  XML config demo не собран, пропускаем\033[0m"
    fi
    echo ""

    echo -e "\033[1;36m=== Демо 7: Создание из XML ===\033[0m"
    if [ -f "./create_from_xml" ]; then
        if [ -f "containers_config.xml" ]; then
            LD_LIBRARY_PATH=../../userlib/build ./create_from_xml containers_config.xml
        else
            echo -e "\033[1;33m⚠️  Файл containers_config.xml не найден\033[0m"
        fi
    else
        echo -e "\033[1;33m⚠️  Create from XML не собран, пропускаем\033[0m"
    fi
    echo ""
fi

echo "================================================"
echo -e "\033[1;32m🎉 Все демонстрации завершены!\033[0m"
echo -e "\033[1;33m📊 Текущие контейнеры:\033[0m"
if [ -f "./container_monitor" ]; then
    LD_LIBRARY_PATH=../../userlib/build ./container_monitor
else
    echo -e "\033[1;33m⚠️  Container monitor не доступен\033[0m"
fi
echo "================================================"

echo -e "\033[1;35m💡 Для очистки выполните: sudo rmmod kcontainer\033[0m"
echo -e "\033[1;35m💡 Для принудительного удаления контейнеров используйте force_destroy()\033[0m"
echo "================================================"