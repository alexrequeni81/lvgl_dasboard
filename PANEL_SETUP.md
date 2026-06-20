# Panel Setup — IEI AFL-08AH

## Arquitectura
- **HW**: IEI AFL-08AH (Atom N270, 800×600)
- **SO**: Tiny Core Linux 15.x (i686)
- **App**: LVGL 9.1 sobre FBDEV + EVDEV
- **Táctil**: PenMount serie (ttyS3) vía inputattach
- **Audio**: Intel ICH7 + AC'97 (ALSA)
- **Red**: Ethernet + WiFi RT73 USB

## Flujo de arranque (bootlocal.sh)
```
1. modprobe serio_raw
2. inputattach --penmount9000 /dev/ttyS3  →  event6
3. ntpclient -h pool.ntp.org -s           →  hora correcta
4. amixer set PCM 70%                     →  volumen inicial
5. dashboard_app                          →  LVGL sobre /dev/fb0
   ├── NTP sync (de nuevo, en proceso)
   ├── Geo-IP → ip-api.com → ciudad, TZ, lat/lon
   ├── weather_init → carga cache o fetch Open-Meteo
   └── UI loop (touch + clock + radio + weather)
```

## Configuración inicial en TC
```sh
# Como root en el panel:
tce-load -wi curl ca-certificates mpg123 alsa alsa-config ntpclient inputattach
amixer sset Master 70% unmute
amixer sset PCM 70% unmute
sudo alsactl store
```

## Auto-arranque
`/opt/bootlocal.sh` (ver setup_tc.sh)

## Actualización desde Windows
```bat
update_panel.bat
```

## Atajos útiles
```sh
# Restaurar terminal tras cerrar app
sudo chvt 1 && sudo chvt 2 && clear

# Probar táctil
cat /dev/input/event6 | hexdump -C

# Probar audio
speaker-test -t sine -f 880 -l 1

# Probar radio
mpg123 -v "https://streaming.enacast.com:8000/radioribarroja128.mp3"

# Sync hora manual
ntpclient -h pool.ntp.org -s

# Logs de la app
./dashboard_app 2>&1 | tee /tmp/dashboard.log
```

## Estructura del proyecto
```
dashboard/
├── main.c                    # Entry point, init, NTP, geo-IP
├── lv_conf.h                 # LVGL config (FBDEV 32bpp / SDL2 16bpp)
├── build_cross.sh            # Cross-compile CI script
├── patches/lvgl-fixes.patch  # Parches a LVGL
├── update_panel.bat/.ps1     # Actualización Windows → TC
├── setup_tc.sh               # Setup inicial en TC
└── src/
    ├── ui/dashboard_tab.c    # UI principal (reloj, radio, weather)
    ├── ui/calendar_tab.c     # Agenda modal
    ├── time_manager.c        # Alarma + settings.conf
    ├── weather.c             # Open-Meteo API + geo-IP
    ├── radio_manager.c       # mpg123 (Linux) / ffplay (Win)
    ├── net_manager.c         # HTTP (curl Linux / WinHTTP Win)
    └── ...
```
