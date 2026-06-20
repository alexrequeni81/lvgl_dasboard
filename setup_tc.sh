#!/bin/sh
# setup_tc.sh — Configuración completa del panel IEI AFL-08AH con Tiny Core Linux
# Ejecutar como: sudo sh setup_tc.sh
set -e

echo "=== Setup Panel Dashboard TC ==="
echo ""

# ── 1. Paquetes ──
echo "[1/8] Instalando paquetes..."
tce-load -wi curl ca-certificates
tce-load -wi mpg123 alsa alsa-config alsa-modules-6.18.2-tinycore
tce-load -wi ntpclient
tce-load -wi inputattach
tce-load -wi openssh

# ── 2. SSH key-based auth (opcional) ──
echo "[2/8] Configurando SSH..."
mkdir -p /home/tc/.ssh
chmod 700 /home/tc/.ssh
# Si existe clave pública en /mnt/sda1, copiarla
if [ -f /mnt/sda1/authorized_keys ]; then
    cp /mnt/sda1/authorized_keys /home/tc/.ssh/
    chmod 600 /home/tc/.ssh/authorized_keys
    chown -R tc:staff /home/tc/.ssh
    sed -i 's/^#PermitEmptyPasswords no/PermitEmptyPasswords no/' /usr/local/etc/ssh/sshd_config
    echo "   Clave SSH copiada desde /mnt/sda1"
fi

# ── 3. ALSA ──
echo "[3/8] Configurando ALSA..."
sudo alsactl init 2>/dev/null || true
# Subir volúmenes y guardar
amixer sset Master 70% unmute 2>/dev/null
amixer sset PCM 70% unmute 2>/dev/null
amixer sset Surround 70% unmute 2>/dev/null
sudo alsactl store
echo "   Volumen ALSA configurado al 70%"

# ── 4. Módulos de kernel ──
echo "[4/8] Configurando módulos de kernel..."
# Añadir módulos para que se carguen al arrancar
echo "serio_raw" >> /etc/sysconfig/optional-modules 2>/dev/null || true
echo "snd-intel8x0" >> /etc/sysconfig/optional-modules 2>/dev/null || true

# ── 5. bootlocal.sh ──
echo "[5/8] Creando /opt/bootlocal.sh..."
cat > /opt/bootlocal.sh << 'EOF'
#!/bin/sh
# bootlocal.sh — Arranque del panel dashboard

# Cargar módulo serie para táctil PenMount
modprobe serio_raw 2>/dev/null

# Iniciar driver táctil PenMount (puerto serie ttyS3)
/usr/local/sbin/inputattach --penmount9000 /dev/ttyS3 &
sleep 1

# Sincronizar hora por NTP (no bloqueante)
/usr/local/bin/ntpclient -h pool.ntp.org -s >/dev/null 2>&1 &

# Subir volumen ALSA
amixer sset PCM 70% unmute >/dev/null 2>&1

# Iniciar dashboard (esperar a que la red esté lista)
sleep 5
/home/tc/dashboard_app </dev/null >/dev/null 2>&1 &
EOF
chmod +x /opt/bootlocal.sh
echo "   Creado /opt/bootlocal.sh"

# ── 6. Configuración de red WiFi (si aplica) ──
echo "[6/8] Configurando WiFi (si hay)..."
if [ -f /mnt/sda1/wifi.conf ]; then
    . /mnt/sda1/wifi.conf
    # SSID y PASSWORD deben estar definidos en wifi.conf
    if [ -n "$SSID" ] && [ -n "$PASSWORD" ]; then
        cat > /opt/wifi.sh << 'SCRIPT'
#!/bin/sh
# WiFi auto-connect
SSID="__SSID__"
PASSWORD="__PASSWORD__"
ifconfig wlan0 up 2>/dev/null
iwconfig wlan0 essid "$SSID" key "s:$PASSWORD" 2>/dev/null
udhcpc -b -i wlan0 -p /var/run/udhcpc.wlan0.pid >/dev/null 2>&1 &
SCRIPT
        sed -i "s/__SSID__/$SSID/; s/__PASSWORD__/$PASSWORD/" /opt/wifi.sh
        chmod +x /opt/wifi.sh
        echo "   WiFi configurado para SSID: $SSID"
    fi
else
    echo "   No se encontró wifi.conf en /mnt/sda1 (opcional)"
fi

# ── 7. Descargar última versión de la app ──
echo "[7/8] Descargando última versión de dashboard_app..."
if [ -f /home/tc/dashboard_app ]; then
    echo "   Ya existe /home/tc/dashboard_app, omitiendo."
    echo "   Para actualizar: usa update_panel.bat desde Windows"
else
    # Intentar descargar desde GitHub CI
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
        echo "   ERROR: No se pudo descargar. Copia dashboard_app manualmente con SCP"
    fi
fi

# ── 8. Guardar configuración persistente ──
echo "[8/8] Guardando configuración persistente..."
filetool.sh -b
echo ""
echo "=== Setup completado ==="
echo ""
echo "Resumen:"
echo "  - Paquetes instalados: curl, mpg123, alsa, ntpclient, inputattach, openssh"
echo "  - ALSA configurado al 70%"
echo "  - /opt/bootlocal.sh creado (touch + NTP + auto-arranque)"
echo "  - SSH configurado"
echo "  - Dashboard descargado (o pendiente de copiar)"
echo ""
echo "Próximos pasos:"
echo "  1. sudo reboot"
echo "  O desde Windows:"
echo "  - Copiar dashboard_app: scp dashboard_app tc@192.168.1.246:/home/tc/"
echo "  - Actualizar:        .\\update_panel.bat"
echo ""
