# Estado del Proyecto: Dashboard Industrial AFL-08AH

**Fecha:** 13 Jun 2026 | **Repo:** https://github.com/alexrequeni81/lvgl_dasboard

---

## Estado Actual

El CI compila correctamente un binario **ELF 32-bit LSB i686, estático, 1.8 MB** listo para Tiny Core Linux. Queda probarlo en el target.

---

## Flujo de Trabajo

```
Windows (editar + test local)
       │
       │ git push
       ▼
GitHub Actions (Cross-Compile i686)
       │
       │ build_cross.sh → dashboard_app estático
       │
       ▼
Artifact descargable (dashboard_app-linux-i686.zip)
       │
       │ opcional: wget/scp al Tiny Core
       ▼
Panel IEI AFL-08AH (Tiny Core Linux)
```

### 1. Desarrollo Local (Windows)
```powershell
# Ya tienes el build configurado con CMake + Ninja + SDL2
cmake -B build_win -G Ninja
ninja -C build_win
./build_win/dashboard_app   # Simulador SDL2 en 800×600
```

### 2. Publicar y Compilar
```bash
git add .; git commit -m "mensaje"
git push origin main
# GitHub Actions compila automáticamente (~30s)
# Ve a: https://github.com/alexrequeni81/lvgl_dasboard/actions
#      → Última run → Artifacts → dashboard_app-linux-i686
```

### 3. Desplegar en Tiny Core (Manual)
```sh
# Descargar el artifact desde GitHub (URL de descarga directa)
wget -O /home/tc/dashboard_app <URL>

# O desde otro PC con SCP
scp dashboard_app tc@<IP>:/home/tc/

chmod +x /home/tc/dashboard_app
echo 0 > /sys/class/graphics/fbcon/cursor_blink
/home/tc/dashboard_app
```

### 4. Despliegue Automático (Opcional)
Se puede añadir un paso al workflow que suba el binario vía SCP/rsync al panel si tiene IP fija:

```yaml
# .github/workflows/build.yml (paso opcional)
- name: Deploy to Tiny Core
  run: |
    scp build_linux/dashboard_app tc@${{ secrets.TC_IP }}:/home/tc/
  if: github.ref == 'refs/heads/main'
```

---

## CI: Cómo Compila (build_cross.sh)

El script `build_cross.sh` en la raíz:
```
i686-linux-gnu-gcc -Os -std=c99 -D_GNU_SOURCE
  -DLV_CONF_PATH=/path/lv_conf.h
  -I. -I./lvgl
  -static -lm -lpthread -lrt
  main.c src/*.c lvgl/src/*.c (excluye: SDL, Windows, X11, NuttX, libs opcionales)
```

**Dependencias CI (apt):** `gcc-i686-linux-gnu libc6-dev-i386-cross`

---

## Estado por Módulo

| Módulo | Estado |
|--------|--------|
| Dashboard principal (reloj, clima, radio, alarmas) | ✅ |
| Calendario/Agenda con persistencia JSON | ✅ |
| Editor de reloj modal con teclado numérico | ✅ |
| Calculadora | 🟡 Esqueleto |
| Conversores | 🟡 Esqueleto |
| Simulador Windows (SDL2) | ✅ |
| Cross-compile CI (GitHub Actions) | ✅ |
| Prueba en Tiny Core Linux | 🔴 Pendiente |

---

## Próximos Pasos

1. **Probar en Tiny Core** — descargar artifact y ejecutar en el panel
2. **Ajustar touch** — calibrar `/dev/input/eventX` si es necesario
3. **Implementar Calculadora** y **Conversores**
4. **Inicio automático** en TC (`/opt/bootlocal.sh`)
5. **Despliegue automático** vía SCP en el CI (opcional)
