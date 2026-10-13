"""Resume un registro del servidor: py -3.13 -m tools.analizar_registro docs/evidencias/p2/servidor.log

Calcula lo que piden PN-01 (operacion continua) y PN-04 (lecturas por segundo).
"""

import argparse
from datetime import datetime
from pathlib import Path
import re

LINEA = re.compile(r"^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3}) \w+ (.*)$")


def resumir(lineas) -> dict:
    posts = []
    resumen = {"invalidas": 0, "set": 0, "registros_sensor": 0, "registros_actuador": 0,
               "errores": 0, "cierres_sin_post": 0}
    for linea in lineas:
        coincidencia = LINEA.match(linea.strip())
        if coincidencia is None:
            continue
        instante = datetime.strptime(coincidencia.group(1), "%Y-%m-%d %H:%M:%S,%f")
        mensaje = coincidencia.group(2)
        if mensaje.startswith("POST distance="):
            posts.append(instante)
            resumen["invalidas"] += mensaje.endswith("=invalid")
        elif mensaje.startswith("SET "):
            resumen["set"] += 1
        elif mensaje == "REGISTER sensor aceptado":
            resumen["registros_sensor"] += 1
        elif mensaje == "REGISTER actuator aceptado":
            resumen["registros_actuador"] += 1
        elif mensaje.startswith("ERROR "):
            resumen["errores"] += 1
        elif mensaje.startswith("2000 ms sin POST"):
            resumen["cierres_sin_post"] += 1
    resumen["posts"] = len(posts)
    resumen["inicio"] = posts[0] if posts else None
    resumen["fin"] = posts[-1] if posts else None
    resumen["duracion_s"] = (posts[-1] - posts[0]).total_seconds() if len(posts) > 1 else 0.0
    intervalos = [(b - a).total_seconds() for a, b in zip(posts, posts[1:])]
    resumen["mayor_intervalo_s"] = max(intervalos, default=0.0)
    # n lecturas ocupan n - 1 intervalos dentro de la ventana medida.
    resumen["lecturas_por_s"] = (len(posts) - 1) / resumen["duracion_s"] if resumen["duracion_s"] else 0.0
    return resumen


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Resume el registro guardado con --registro.")
    parser.add_argument("archivo", type=Path)
    args = parser.parse_args(argv)
    r = resumir(args.archivo.read_text(encoding="utf-8").splitlines())
    if not r["posts"]:
        print("El registro no contiene lecturas POST.")
        return 1
    print(f"Ventana de lecturas: {r['inicio']:%H:%M:%S} -> {r['fin']:%H:%M:%S} ({r['duracion_s']:.1f} s)")
    print(f"POST recibidos: {r['posts']} ({r['lecturas_por_s']:.2f} lecturas/s), invalidos: {r['invalidas']}")
    print(f"SET enviados: {r['set']}")
    print(f"Mayor intervalo entre POST: {r['mayor_intervalo_s']:.2f} s")
    print(f"Registros aceptados: sensor {r['registros_sensor']}, actuador {r['registros_actuador']}")
    print(f"Cierres por 2 s sin POST: {r['cierres_sin_post']}; ERROR enviados: {r['errores']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
