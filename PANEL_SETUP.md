# Panel Setup — IEI AFL-08AH

## Hardware
- Panel IEI AFL-08AH (Atom N270, 512 MB RAM)
- Display 800×600 táctil resistivo (PenMount 9000 serie)
- Audio AC'97 ALC655 (altavoces internos)
- Ethernet 2× (eth0 sin conectar, eth1 activa)

## Tiny Core Linux

### IP
```
IP: 192.168.1.246
User: tc / root
Pass: root
```

### Archivos clave
| Archivo | Propósito |
|---------|-----------|
| `/opt/bootlocal.sh` | Script de arranque |
| `/home/tc/dashboard_app` | Binario de la app |
| `/home/tc/inputattach` | Driver táctil (no disponible en repo TC) |
| `/opt/.filetool.lst` | Lista para backup persistente |
| `/etc/sysconfig/optional-modules` | Módulos de kernel a cargar |
| `/etc/sysconfig/timezone` | Timezone |

### Backup
```sh
filetool.sh -b   # guarda mydata.tgz
```

### ALSA
```sh
amixer sset Master 70% unmute
amixer sset PCM 70% unmute
alsactl store
```

### NTP
```sh
ntpclient -h pool.ntp.org -s
```

### Radio (SSL requiere hora correcta)
```sh
mpg123 "https://streaming.enacast.com:8000/radioribarroja128.mp3"
```

### Deploy desde Windows
```bat
update_panel.bat
```
