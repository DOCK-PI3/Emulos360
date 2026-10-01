# Ajustes nativos

El catálogo contiene 282 opciones persistentes del motor Netplay y las extensiones
de Emulos360, más 22 controles de consola: 304 entradas en total (revisión 0.13).
Se actualiza con `scripts/update-netplay-catalog.py`, que lee tanto `.cc` como `.cpp`.
Las opciones Live se agrupan en Red; la ruta externa está en Almacenamiento.
Los flags transitorios de diagnóstico y lanzamiento no son preferencias; el
puente de perfiles usa los suyos internamente. Menús de acción como abrir un
archivo tampoco son ajustes persistentes.

Las categorías son General, Gráficos, Pantalla, Audio, Mandos, Teclado, Consola,
Perfiles, Red, Almacenamiento, Sistema, CPU y memoria, Menús del motor, Registros
y Avanzados. Emulos360 agrupa por separado sus preferencias de biblioteca y
pantalla completa. La búsqueda incluye etiqueta, clave técnica y descripción.

Cada control permite ver la clave original, descripción del motor y valor
predeterminado. Los controles habituales están traducidos; las descripciones
técnicas originales se conservan en inglés para no alterar su significado.
Los campos validan tipos, enumeraciones y límites conocidos. Los valores de
64 bits se transportan como texto para no perder precisión en JavaScript.

**Guardar** aplica el borrador al TOML y, cuando corresponda, a `xconfig.settings`.
Los cambios se utilizan en la siguiente sesión. **Descartar** vuelve a leer el
disco. **Restaurar sección** carga los valores predeterminados en el borrador;
todavía requiere Guardar. **Importar TOML** también deja un borrador y
**Exportar** escribe ese borrador en el archivo elegido, incluyendo claves
desconocidas. No exporta el archivo binario de consola.

Se conservan copias `.bak` y se detectan modificaciones externas antes de
guardar. No se editan los archivos mientras el motor está activo. Para cambiar
la carpeta de datos y opciones de consola, guarda primero la ruta y después
edita la consola: así se evita escribir el XConfig de la ubicación equivocada.

Generación del catálogo:

```powershell
python scripts/generate-settings-catalog.py local/xenia-baseline/xenia-canary.config.toml
```

Ese archivo de referencia corresponde a la base ya fijada. Los metadatos propios
están en `app/settings-labels.json`; los valores y comentarios de Xenia proceden
del TOML de referencia y sus declaraciones C++.
