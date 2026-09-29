# Terraforge

Редактор карт для фэнтези-миров на C++20: генерирует континент процедурно, а потом
позволяет «лепить» рельеф кистями — поднимать горы, копать проливы, сглаживать
и выравнивать землю. Готовую карту можно сохранить в PNG.

![Terraforge](docs/screenshot.png)

## Возможности

- Процедурная генерация рельефа: фрактальный шум (fBm, OpenSimplex2) и маска формы мира,
  одинаковый seed всегда даёт одинаковую карту
- Форма континента: заготовки (континент, архипелаг, два материка, внутреннее море, пустой океан)
  или свой контур кистями «Суша» / «Море» — рельеф пересобирается на лету
- Кисти: поднять, опустить, сгладить, выровнять; мягкий край, скорость не зависит от FPS
- Гидравлическая эрозия каплями дождя (Beyer 2015): долины, русла, конусы выноса
- Реки и озёра: заполнение впадин Priority-Flood (Barnes 2014) и накопление стока
- Климат и биомы: температура по широте и высоте, влажность от моря, рек и ветра с дождевой тенью;
  ледники, тундра, тайга, леса, луга, степи, пустыни, болота, горы
- Горные перевалы: седловины между долинами, найденные деревом слияний (union-find)
- Уровень моря, отмывка рельефа (освещение с северо-запада, как на бумажных картах), береговая линия,
  карта рельефа или биомов
- Зум к курсору, перемещение карты, экспорт в PNG
- Интерфейс на английском и русском (меню «Language / Язык»), выбор сохраняется между запусками;
  полнота переводов проверяется при компиляции (`consteval`)
- Ядро без зависимостей от графики, покрыто юнит-тестами (GoogleTest), CI на Windows и Linux

Что будет дальше — миры в SQLite и стартовое меню, 3D-просмотр, undo/redo,
города, королевства и дороги, генерация лора нейросетью — в [ROADMAP.md](ROADMAP.md).

## Сборка

### Windows

Нужно один раз установить:

1. **Visual Studio 2022 или новее** (Community бесплатна) с нагрузкой
   «Разработка классических приложений на C++».
2. **CMake** и **Git**, чтобы они были доступны в терминале (после установки откройте новый терминал):
   ```powershell
   winget install Kitware.CMake
   winget install Git.Git
   ```

Сборка и запуск из терминала в папке проекта:

```powershell
cmake -S . -B build                      # первый раз скачивает библиотеки, это несколько минут
cmake --build build --config Release
ctest --test-dir build -C Release        # тесты
# замеры скорости (отключены в ctest):
.\build\bin\Release\terraforge_tests.exe --gtest_also_run_disabled_tests --gtest_filter=Benchmark.*
.\build\bin\Release\terraforge.exe
```

Или откройте папку проекта в Visual Studio («Открыть локальную папку»), выберите
`terraforge.exe` как элемент запуска и нажмите F5. Для плавной работы используйте
конфигурацию Release, Debug нужен для отладки.

### Linux

```bash
sudo apt install build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build
./build/bin/terraforge
```

## Управление

| Действие | Как |
|---|---|
| Рисовать кистью | левая кнопка мыши |
| Поднять ↔ опустить | Shift + левая кнопка |
| Двигать карту | правая или средняя кнопка мыши |
| Зум | колесо мыши |
| Размер кисти | Ctrl + колесо или `[` / `]` |
| Инструменты | `1` поднять, `2` опустить, `3` сгладить, `4` выровнять, `5` суша, `6` море |
| Суша ↔ море | Shift + левая кнопка с инструментом «Суша» или «Море» |
| Вписать карту в окно | `F` |
| Карта рельефа ↔ биомов | `M` |
| Генерация рельефа, форма мира | меню «Файл» или кнопка внизу панели «Свойства» |
| Эрозия, реки, климат, перевалы | окно «География» (меню «Файл» или панель «Свойства») |
| Язык интерфейса | меню «Language / Язык» |

## Устройство проекта

```
src/core/    данные и алгоритмы (без графики): Heightmap, TerrainGenerator, ContinentShape, Brush,
             Erosion, Hydrology, Climate, MountainPasses, Geography, MapColorizer,
             Localization (переводы EN/RU), AppSettings (terraforge.ini)
src/app/     окно, ввод, рендер и интерфейс: App (цикл и ввод), AppUi (меню и панели),
             MapRenderer, Window
assets/      файлы, которые копируются рядом с .exe: шрифт интерфейса
tests/       юнит-тесты ядра (GoogleTest)
cmake/       подключение зависимостей и флаги предупреждений
```

Ядро не знает о raylib и ImGui, поэтому алгоритмы тестируются без окна,
а графическую часть можно заменить, не трогая логику.

## Стек

C++20 · CMake · [raylib](https://www.raylib.com) · [Dear ImGui](https://github.com/ocornut/imgui)
· [rlImGui](https://github.com/raylib-extras/rlImGui) · [FastNoiseLite](https://github.com/Auburn/FastNoiseLite)
· [GoogleTest](https://github.com/google/googletest) · GitHub Actions

Библиотеки скачиваются при сборке и распространяются под своими лицензиями
(zlib, MIT, BSD-3-Clause). Шрифт интерфейса — [Noto Sans](https://notofonts.github.io)
под лицензией SIL Open Font License 1.1 ([assets/fonts/OFL.txt](assets/fonts/OFL.txt)),
иконки — Font Awesome Free (входит в rlImGui).
