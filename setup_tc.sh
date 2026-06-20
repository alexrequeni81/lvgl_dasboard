#!/bin/sh
# setup_tc.sh — Configuración completa del panel IEI AFL-08AH con Tiny Core Linux
# Ejecutar como: sudo sh setup_tc.sh
set -e

echo "=== Setup Panel Dashboard TC ==="
echo ""

# ── 0. Limpiar paquetes de desarrollo (solo necesario una vez) ──
echo "[0/8] Eliminando paquetes de desarrollo innecesarios..."
tce-audit delete compiletc cmake gcc gcc_base-dev gcc_libs gcc_libs-dev glibc_base-dev 2>/dev/null || true
tce-audit delete perl5 bison flex m4 make patch diffutils findutils sed 2>/dev/null || true
tce-audit delete fltk-1.3 libX11 libXau libXcursor libXext libXfixes libXrender libxcb 2>/dev/null || true
tce-audit delete xf86-input-libinput 2>/dev/null || true
tce-audit delete firmware-atheros firmware-ipw2100 firmware-ipw2200 firmware-iwimax 2>/dev/null || true
tce-audit delete firmware-iwl8000 firmware-iwl9000 firmware-iwlwifi firmware-marvel 2>/dev/null || true
tce-audit delete firmware-myri10ge firmware-netxen firmware-openfwwf firmware-ralinkwifi 2>/dev/null || true
tce-audit delete firmware-rtlwifi firmware-ti-connectivity firmware-ueagle-atm firmware-vxge 2>/dev/null || true
tce-audit delete firmware-zd1211 hwdata 2>/dev/null || true
echo "   Hecho."

# ── 1. Paquetes esenciales ──
echo "[1/8] Instalando paquetes esenciales..."
tce-load -wi curl ca-certificates
tce-load -wi mpg123 alsa alsa-config alsa-modules-6.18.2-tinycore
tce-load -wi ntpclient
tce-load -wi openssh

# ── 2. ALSA ──
echo "[2/8] Configurando ALSA..."
alsactl init 2>/dev/null || true
amixer sset Master 70% unmute 2>/dev/null
amixer sset PCM 70% unmute 2>/dev/null
amixer sset Surround 70% unmute 2>/dev/null
alsactl store 2>/dev/null || sudo alsactl store 2>/dev/null || true
echo "   Volumen ALSA configurado al 70%"

# ── 3. Módulos de kernel ──
echo "[3/8] Configurando módulos de kernel..."
echo "serio_raw" >> /etc/sysconfig/optional-modules 2>/dev/null || sudo sh -c 'echo "serio_raw" >> /etc/sysconfig/optional-modules' 2>/dev/null || true
echo "snd-intel8x0" >> /etc/sysconfig/optional-modules 2>/dev/null || sudo sh -c 'echo "snd-intel8x0" >> /etc/sysconfig/optional-modules' 2>/dev/null || true

# ── 4. bootlocal.sh optimizado ──
echo "[4/8] Creando /opt/bootlocal.sh..."
BOOTLOCAL="/opt/bootlocal.sh"
cat > /tmp/bootlocal.sh << 'BOOTEOF'
#!/bin/sh
# bootlocal.sh — Arranque del panel dashboard

export TZ=Europe/Madrid

# Red (Ethernet) — auto-detect interfaz
udhcpc -b >/dev/null 2>&1

# Táctil PenMount (puerto serie ttyS3)
modprobe serio_raw 2>/dev/null
/home/tc/inputattach --daemon --penmount9000 /dev/ttyS3

# Enlace mouse (compatibilidad)
sleep 2
rm -f /dev/mouse
ln -s /dev/input/mice /dev/mouse

# SSH
if [ ! -f /usr/local/etc/ssh/ssh_config ]; then
    cp /usr/local/etc/ssh/ssh_config.orig /usr/local/etc/ssh/ssh_config 2>/dev/null
    cp /usr/local/etc/ssh/sshd_config.orig /usr/local/etc/ssh/sshd_config 2>/dev/null
fi
echo "root:root" | chpasswd
echo "tc:root" | chpasswd
/usr/local/etc/init.d/openssh start &

# Esperar a que la red esté activa antes de NTP
for i in 1 2 3 4 5; do
    ping -c 1 -W 2 pool.ntp.org >/dev/null 2>&1 && break
    sleep 2
done

# Sincronizar hora por NTP (síncrono, espera resultado)
ntpclient -h pool.ntp.org -s >/dev/null 2>&1

# Volumen ALSA (Master y PCM)
amixer sset Master 70% unmute >/dev/null 2>&1
amixer sset PCM 70% unmute >/dev/null 2>&1
alsactl store >/dev/null 2>&1

# Dashboard — espera breve y lanza
sleep 2
/home/tc/dashboard_app </dev/null >/dev/null 2>&1 &
BOOTEOF
cp /tmp/bootlocal.sh /opt/bootlocal.sh 2>/dev/null || sudo cp /tmp/bootlocal.sh /opt/bootlocal.sh 2>/dev/null
chmod +x /opt/bootlocal.sh 2>/dev/null || sudo chmod +x /opt/bootlocal.sh 2>/dev/null
rm -f /tmp/bootlocal.sh
echo "   Creado /opt/bootlocal.sh"

# ── 5. Descargar última versión de la app (si no existe) ──
echo "[5/8] Descargando última versión de dashboard_app..."
if [ -f /home/tc/dashboard_app ]; then
    echo "   Ya existe /home/tc/dashboard_app, omitiendo."
    echo "   Para actualizar: usa update_panel.bat desde Windows"
else
    curl -sL -o /tmp/dashboard_app.zip \
      https://nightly.link/alexrequeni81/lvgl_dasboard/workflows/build.yml/main/dashboard_app-linux-i686.zip
    if [ -f /tmp/dashboard_app.zip ]; then
        unzip -o /tmp/dashboard_app.zip -d /tmp/
        cp /tmp/dashboard_app /home/tc/dashboard_app 2>/dev/null || true
        rm -f /tmp/dashboard_app.zip
    fi
    if [ -f /home/tc/dashboard_app ]; then
        chmod +x /home/tc/dashboard_app
        echo "   Dashboard descargado correctamente"
    else
        echo "   ERROR: No se pudo descargar. Copia manualmente con SCP"
    fi
fi

# ── 6. Guardar configuración persistente ──
echo "[6/8] Guardando configuración persistente..."
filetool.sh -b 2>/dev/null || sudo filetool.sh -b 2>/dev/null || true

echo ""
echo "=== Setup completado ==="
echo ""
echo "Resumen:"
echo "  - Paquetes de desarrollo eliminados (~100MB liberados)"
echo "  - Paquetes esenciales: curl, mpg123, alsa, ntpclient, inputattach, openssh"
echo "  - ALSA configurado al 70%"
echo "  - /opt/bootlocal.sh: TZ, red, táctil, NTP, ALSA, SSH, auto-arranque"
echo "  - Dashboard listo"
echo ""
echo "Próximos pasos:"
echo "  1. sudo reboot"
echo "  2. Desde Windows: .\\update_panel.bat para actualizar"
echo ""
