#!/bin/bash
# sysinfo.sh - TUGAS 1 OS - KELOMPOK AXX

print_header() {
  local title="TUGAS 1 OS - KELOMPOK B08"
  local width=82

  printf '%*s\n' "$width" '' | tr ' ' '='
  printf "%*s\n" $(( (${#title} + width) / 2 )) "$title"
  printf '%*s\n' "$width" '' | tr ' ' '='
}

# =====================================================
# [HISYAM] Bagian 2 No.1 - sysinfo.sh dasar
# =====================================================

check_os_kernel() {
  local os_name
  os_name=$(grep -oE '^PRETTY_NAME="[^"]*"' /etc/os-release | cut -d'"' -f2)
  
  local kernel_version
  kernel_version=$(uname -r)

  if [ -z "$os_name" ]; then
    os_name="Unknown OS"
  fi

  echo "OS/Kernel         : $os_name (Kernel $kernel_version)"

  OS_INFO="$os_name"
  KERNEL_INFO="$kernel_version"
}

check_users() {
  local user_count

  # Count user with 1000 <= UID < 65534
  user_count=$(awk -F: '$3 >= 1000 && $3 < 65534 {count++} END {print count+0}' /etc/passwd)

  echo "Akun pengguna     : $user_count akun"
  USER_COUNT="$user_count"
}

check_processes() {
  local process_count
  process_count=$(ps -e --no-headers | wc -l)

  echo "Proses berjalan   : $process_count proses"
  PROCESS_COUNT="$process_count"
}

check_virtualization() {
  local virt_type="none"

  if command -v systemd-detect-virt &>/dev/null; then
    virt_type=$(systemd-detect-virt 2>/dev/null)
  fi

  if [ "$virt_type" != "none" ] && [ -n "$virt_type" ]; then
    echo "Virtualisasi      : Terdeteksi ($virt_type)"
    VIRT_DETECTED="yes"
    VIRT_TYPE="$virt_type"
  else
    echo "Virtualisasi      : Tidak terdeteksi (mesin fisik)"
    VIRT_DETECTED="no"
    VIRT_TYPE="none"
  fi
}

# =====================================================
# [ADRIAN] Bagian 2 No.2 - Konektor ke resource_check.c
# =====================================================

send_to_c_connector() {
    # Ambil memory usage (dalam persentase)
    local memory_usage=$(free -b | LC_NUMERIC=C awk '/^Mem:/ {printf "%.2f", ($3/$2)*100}')

    # Ambil load average dan total cores
    local load_average=$(awk '{print $1}' /proc/loadavg)
    local total_cores=$(nproc)

    # Hitung load per core (load average dibagi jumlah core)
    load_per_core=$(LC_NUMERIC=C awk -v load_averages="$load_average" -v cores="$total_cores" 'BEGIN {printf "%.2f", load_averages/cores}')

    if [ -z "$memory_usage" ] || [ -z "$load_per_core" ]; then
      memory_usage=0
      load_per_core=0
    fi

    local hasil=$(echo "$memory_usage $load_per_core" | ./resource_check)

    read -r status_memory status_load <<< "$hasil"

    echo "  Memory Usage(%):             $memory_usage%  [$status_memory]" 
    echo "  Load Average vs jumlah core: $load_per_core    [$status_load]"
}

# =====================================================
# [DIMAS] Bagian 2 No.3 - Fitur tambahan
# =====================================================

EXTRA_FEATURE_NAME="System Uptime"
EXTRA_FEATURE_VALUE="UNKNOWN"

check_extra_feature() {
    EXTRA_FEATURE_VALUE=$(uptime -p | sed 's/^up //')

    echo "[EXTRA] $EXTRA_FEATURE_NAME: $EXTRA_FEATURE_VALUE"
}

# =====================================================
# [FIQHI] Bagian 2 No.4 - Task reporter
# =====================================================

generate_report() {
  local w1=20
  local w2=26
  local w3=9
  local w4=40

  local memory_usage
  memory_usage=$(free -b | LC_NUMERIC=C awk '/^Mem:/ {printf "%.2f", ($3/$2)*100}')

  local virt_detail
  virt_detail=$([ "$VIRT_DETECTED" = "yes" ] && echo "Terdeteksi: $VIRT_TYPE" || echo "Tidak terdeteksi")


  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Check Category" "$w2" "Item" "$w3" "Status" "$w4" "Details"

  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "OS" "$w2" "$OS_INFO" "$w3" "PASS" "$w4" "$KERNEL_INFO"
  
  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Users" "$w2" "Regular Accounts" "$w3" "PASS" "$w4" "$USER_COUNT akun"
  
  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Process" "$w2" "Running" "$w3" "PASS" "$w4" "$PROCESS_COUNT proses berjalan"
  
  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Virtualization" "$w2" "Hypervisor" "$w3" "PASS" "$w4" "$virt_detail"
  
  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Memory" "$w2" "$memory_usage%" "$w3" "$status_memory" "$w4" "Memory usage"

  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'
  printf "| %-*s | %-*s | %-*s | %-*s |\n" "$w1" "Load/Core" "$w2" "$load_per_core" "$w3" "$status_load" "$w4" "Load average / jumlah core"

  printf "+%*s+%*s+%*s+%*s+\n" $((w1+2)) "" $((w2+2)) "" $((w3+2)) "" $((w4+2)) "" | tr ' ' '-'

  echo ""
} > sysinfo_report.txt

# =====================================================
# MAIN 
# =====================================================

main() {
  print_header
  echo "Mengecek sistem..."
  check_os_kernel
  check_users
  check_processes
  check_virtualization

  echo ""
  echo "Menghitung metrik varian kelompok..."
  send_to_c_connector   # <- uncomment setelah Adrian selesai

  echo ""
  echo "Fitur tambahan:"
  check_extra_feature

  echo ""
  echo "Menyimpan laporan ke sysinfo_report.txt..."
  generate_report
  echo "Laporan berhasil disimpan."
}

main
