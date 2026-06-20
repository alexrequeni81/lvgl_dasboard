# Dashboard Industrial — IEI AFL-08AH (Tiny Core Linux)

**Fecha:** 20 Jun 2026 | **Repo:** https://github.com/alexrequeni81/lvgl_dasboard

---

## Arquitectura

```
LVGL 9.1 + SDL2 (Windows dev)
         │
         │ Cross-Compile i686-linux-gnu-gcc -static
         ▼
Dashboard App (~1.9 MB ELF i686 estático)
         │
         │ SCP al panel
         ▼
IEI AFL-08AH — Tiny Core Linux 6.18.2-tinycore
  ┌─ FBDEV 800×600 32bpp (framebuffer)
  ├─ EVDEV /dev/input/event6 (PenMount 9000 táctil)
  ├─ ALSA snd_intel8x0 (AC'97 ALC655)
  ├─ NTP pool.ntp.org
  └─ mpg123 (radio streaming)
```

---

## Estructura de Archivos

```
📁 raíz proyecto
├── main.c                  # Punto de entrada: init FBDEV/EVDEV o SDL2
├── lv_conf.h               # Config LVGL + #ifdef __linux__
├── CMakeLists.txt          # Build multiplataforma
├── build_cross.sh          # Cross-compile CI (i686-linux-gnu-gcc -static)
├── setup_tc.sh             # Configuración completa de Tiny Core
├── update_panel.bat        # Deploy desde Windows (kill → SCP → restart)
├── update_panel.ps1        # Ídem en PowerShell
├── PANEL_SETUP.md          # Guía de referencia del panel
├── walk.md                 # Este archivo
│
├── 📁 src/
│   ├── cJSON.c/h           # Parser JSON ligero
│   ├── net_manager.c/h     # HTTP requests (libcurl en Linux, winhttp en Win)
│   ├── time_manager.c/h    # Gestión de hora, NTP
│   ├── weather.c/h         # Clima Open-Meteo + geo-IP (ip-api.com)
│   ├── radio_manager.c/h   # Radio streaming (mpg123 Linux / ffplay Win)
│   ├── agenda_manager.c/h  # Calendario/Agenda con persistencia JSON
│   ├── wifi_manager.c/h    # Esqueleto WiFi (pendiente implementar)
│   └── 📁 ui/
│       ├── ui.c/h          # Layout principal, tabs, sidebar
│       ├── dashboard_tab.c/h  # Tab de inicio: reloj, clima, radio, alarmas
│       ├── calendar_tab.c/h   # Tab calendario/agenda
│       ├── calc_tab.c/h       # Tab calculadora (esqueleto)
│       └── conv_tab.c/h       # Tab conversores (esqueleto)
│
├── 📁 patches/
│   └── lvgl-fixes.patch    # Parches LVGL: transiciones, event safety, ASM
│
└── 📁 lvgl/                # LVGL 9.1 (submódulo, con parches aplicados)
```

---

## Funcionalidades

| Módulo | Estado |
|--------|--------|
| Reloj analógico/digital con editor modal WYSIWYG | ✅ |
| Clima por Open-Meteo + geo-IP (ip-api.com) | ✅ |
| Radio (12 stations, 4-col grid, mpg123) | ✅ |
| Volumen con botones +/– (táctil resistivo) | ✅ |
| Calendario/Agenda con persistencia JSON | ✅ |
| Calculadora | 🟡 Esqueleto |
| Conversores | 🟡 Esqueleto |
| WiFi desde UI | 🔴 Pendiente |

---

## Stack y Configuración

### Target
- **CPU:** Intel Atom N270 (i686, 32-bit)
- **RAM:** 512 MB
- **Display:** 800×600 @ 60Hz, framebuffer `/dev/fb0` 32bpp
- **Táctil:** PenMount 9000 serie `/dev/ttyS3` → `inputattach` → `/dev/input/event6`
- **Audio:** Intel ICH7 + ALC655, ALSA, PCM+Master
- **Red:** Ethernet `eth1` (eth0 no conectado)
- **OS:** Tiny Core Linux, kernel 6.18.2-tinycore

### Paquetes TC Instalados
`curl ca-certificates mpg123 alsa alsa-config alsa-modules-6.18.2-tinycore ntpclient openssh inputattach`

### Arranque (/opt/bootlocal.sh)
1. `udhcpc -i eth1` — DHCP
2. `inputattach --penmount9000` — táctil
3. `ln -s /dev/input/mice /dev/mouse`
4. SSH (openssh start en paralelo)
5. NTP con loop de reintento (`ntpclient -h pool.ntp.org -s`)
6. ALSA: `Master 70%`, `PCM 70%`
7. Limpiar framebuffer + `chvt 1` + ocultar cursor
8. Dashboard (sleep 1 y lanzar)

### Control de Volumen
- `amixer set Master X% unmute` + `amixer set PCM X% unmute`
- Botones +/– de 48×38 px para táctil resistivo

### Calibración Táctil
- X: 88 → 970  (mapeado a 0 → 799)
- Y: 978 → 70  (mapeado a 0 → 599)

### Timezone
- `Europe/Madrid` vía `setenv("TZ", ...)` + `tzset()` en C, y `export TZ` en bootlocal

---

## Flujo de Trabajo

```
Windows (editar + test SDL2)
  │ git push
  ▼
GitHub Actions → build_cross.sh → dashboard_app (i686 estático)
  │
  │ download artifact
  ▼
update_panel.bat → SCP al panel → restart
```

### CI Cross-Compile
```bash
i686-linux-gnu-gcc -Os -std=c99 -static -lm -lpthread -lrt
  -DLV_CONF_PATH=lv_conf.h
  main.c src/*.c lvgl/src/**/*.c
```

### Deploy Windows
```bat
update_panel.bat       # kill SSH → SCP → restart dashboard
```

---

## Parches LVGL (patches/lvgl-fixes.patch)

1. **`lv_obj_style.c:828-843`** — `_lv_obj_style_remove_transitions_for_obj()`: limpia transiciones al destruir objeto (crash PRESSED)
2. **`lv_obj_style.h:367`** — declaración de la función anterior
3. **`lv_obj.c:370`** — llamada a la función en destructor
4. **`lv_obj_event.c:362-366`** — event safety: no procesar eventos si `user_data` es NULL
5. **`custom.cmake`** — ASM solo para ARM (evita error de ensamblador en i686)

---

## Próximos Pasos

1. [ ] Implementar Calculadora (`calc_tab.c`)
2. [ ] Implementar Conversores (`conv_tab.c`)
3. [ ] Implementar WiFi desde UI (`wifi_manager.c`)
4. [ ] Probar estabilidad en producción (24/7)
5. [ ] Agregar pantalla de bloqueo/pausa
