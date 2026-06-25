#!/usr/bin/env bash
set -u

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="${PROJECT_DIR}/benchmark_results"
COUNTS="${PARTICLE_COUNTS:-1024 4096 8192}"
SAMPLES="${SAMPLES:-10}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-180}"

mkdir -p "${RESULTS_DIR}"

SUMMARY_CSV="${RESULTS_DIR}/resumen_cpu_cuda.csv"
SUMMARY_MD="${RESULTS_DIR}/resumen_cpu_cuda.md"

CPU_MODEL="Intel Core i5-11400F"
CPU_CORES="6 nucleos / 12 hilos"
GPU_MODEL="NVIDIA GeForce RTX 4060"
GPU_CORES="3072 nucleos CUDA"

printf "N,cpu_fps,cpu_ms,cuda_fps,cuda_ms,speedup_cuda_vs_cpu\n" > "${SUMMARY_CSV}"

cat > "${SUMMARY_MD}" <<EOF
# Benchmark CPU vs CUDA

Equipo usado:

- CPU: ${CPU_MODEL} (${CPU_CORES})
- GPU: ${GPU_MODEL} (${GPU_CORES})

Configuracion:

- Ventana: \`Config::WINDOW_WIDTH x Config::WINDOW_HEIGHT\`
- Muestras por ejecutable: ${SAMPLES}
- Se mide frame completo: input + simulacion + copia/render OpenGL.

| N | CPU FPS | CPU ms | CUDA FPS | CUDA ms | Speedup |
|---:|---:|---:|---:|---:|---:|
EOF

collect_samples() {
    local label="$1"
    local binary="$2"
    local count="$3"
    local raw_file="${RESULTS_DIR}/${label}_N${count}.csv"
    local log_file="${RESULTS_DIR}/${label}_N${count}.log"

    printf "sample,ms,fps\n" > "${raw_file}"
    : > "${log_file}"

    (
        cd "${PROJECT_DIR}" || exit 1
        timeout "${TIMEOUT_SECONDS}s" "./build/${binary}"
    ) | tee "${log_file}" | awk -v samples="${SAMPLES}" -v out="${raw_file}" '
        /frame=/ && /FPS=/ {
            line = $0
            sub(/^.*frame=/, "", line)
            split(line, frameParts, " ms")
            ms = frameParts[1]

            line = $0
            sub(/^.*FPS=/, "", line)
            split(line, fpsParts, " ")
            fps = fpsParts[1]

            count += 1
            printf "%d,%s,%s\n", count, ms, fps >> out
            if (count >= samples) {
                exit 0
            }
        }
    '

    local collected
    collected="$(awk 'NR > 1 { count += 1 } END { print count + 0 }' "${raw_file}")"

    if [ "${collected}" -lt "${SAMPLES}" ]; then
        echo "[WARN] ${label} N=${count}: se recolectaron ${collected}/${SAMPLES} muestras." >&2
    fi

    awk -F, 'NR > 1 { ms += $2; fps += $3; count += 1 } END {
        if (count == 0) {
            printf "0,0"
        } else {
            printf "%.4f,%.4f", fps / count, ms / count
        }
    }' "${raw_file}"
}

for count in ${COUNTS}; do
    echo "[INFO] Compilando N=${count}"
    make -B -C "${PROJECT_DIR}" PARTICLE_COUNT="${count}" all3

    echo "[INFO] Ejecutando CPU N=${count}"
    cpu_result="$(collect_samples "cpu" "sandboxCPU" "${count}")"
    cpu_fps="${cpu_result%,*}"
    cpu_ms="${cpu_result#*,}"

    echo "[INFO] Ejecutando CUDA N=${count}"
    cuda_result="$(collect_samples "cuda" "sandboxCUDA" "${count}")"
    cuda_fps="${cuda_result%,*}"
    cuda_ms="${cuda_result#*,}"

    speedup="$(awk -v cuda="${cuda_fps}" -v cpu="${cpu_fps}" 'BEGIN {
        if (cpu <= 0) {
            printf "0.0000"
        } else {
            printf "%.4f", cuda / cpu
        }
    }')"

    printf "%s,%s,%s,%s,%s,%s\n" \
        "${count}" "${cpu_fps}" "${cpu_ms}" "${cuda_fps}" "${cuda_ms}" "${speedup}" \
        >> "${SUMMARY_CSV}"

    printf "| %s | %.2f | %.2f | %.2f | %.2f | %.2fx |\n" \
        "${count}" "${cpu_fps}" "${cpu_ms}" "${cuda_fps}" "${cuda_ms}" "${speedup}" \
        >> "${SUMMARY_MD}"
done

echo "[OK] Resumen CSV: ${SUMMARY_CSV}"
echo "[OK] Resumen Markdown: ${SUMMARY_MD}"
