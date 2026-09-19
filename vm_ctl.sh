#!/bin/bash

# Header Printer
print_header() {
  echo "==================================="
  echo "TUGAS 1 OS - KELOMPOK B08"
  echo "==================================="
}

# List Virtual Machines
list_vms() {
    print_header

    echo " Memindai daftar Virtual Machine..."

    if ! VBoxManage list vms | grep -q .; then
        echo " Belum ada Virtual Machine yang terdaftar."
        return
    fi

    echo " Daftar Virtual Machine:"
    VBoxManage list vms | awk -F'"' '{print $2}' | while read vm; do
    echo " - $vm"
    
    done
}

# Get Info
info() {
    print_header
    
    if VBoxManage list vms | awk -F'"' '{print $2}' | grep -Fxq "$2"; then
        vm="$2"
        echo "VM Ditemukan: $vm"
        
                
    else
        echo "VM tidak ditemukan"
        return
    fi

    echo "Memindai RAM dan vCPU"
    memory=$(VBoxManage showvminfo "$vm" --machinereadable | grep '^memory=' | cut -d'=' -f2)
    vcpu=$(VBoxManage showvminfo "$vm" --machinereadable | grep '^cpus=' | cut -d'=' -f2)
    status=$(VBoxManage showvminfo "$vm" --machinereadable | grep '^VMState=' | cut -d'=' -f2 | tr -d '"')

    echo "RAM: $memory MB"
    echo "vCPU: $vcpu"
    echo "status: $status"
}

# Start VM
vm_start() {
  local vm_name="$1"
  print_header
  echo "Menyalakan VM '$vm_name' secara headless..."

  VBoxManage startvm "$vm_name" --type headless

  if [ $? -ne 0 ]; then
    echo "Gagal menyalakan VM '$vm_name'."
    exit 1
  fi

  echo "Menunggu VM siap sepenuhnya (Guest Additions aktif)..."
  local timeout=60
  local elapsed=0
  while [ $elapsed -lt $timeout ]; do
    ga_status=$(VBoxManage guestproperty get "$vm_name" "/VirtualBox/GuestInfo/OS/LoggedInUsersList" 2>/dev/null)
    if [[ "$ga_status" != *"No value set"* ]]; then
      break
    fi
    sleep 2
    elapsed=$((elapsed + 2))
  done

  status=$(VBoxManage showvminfo "$vm_name" --machinereadable | grep '^VMState=' | cut -d'"' -f2)
  echo "VM '$vm_name' berhasil dinyalakan. Status: $status"
}

# Stop VM
vm_stop() {
  local vm_name="$1"
  print_header

  # Cek dulu VM benar-benar running, bukan sedang starting/paused
  status=$(VBoxManage showvminfo "$vm_name" --machinereadable | grep '^VMState=' | cut -d'"' -f2)
  if [ "$status" != "running" ]; then
    echo "VM '$vm_name' tidak dalam status running (status saat ini: $status)."
    exit 1
  fi

  echo "Mematikan VM '$vm_name' secara aman..."
  VBoxManage controlvm "$vm_name" acpipowerbutton

  local timeout=60
  local elapsed=0
  while [ $elapsed -lt $timeout ]; do
    status=$(VBoxManage showvminfo "$vm_name" --machinereadable | grep '^VMState=' | cut -d'"' -f2)
    if [ "$status" == "poweroff" ]; then
      echo "VM '$vm_name' berhasil dimatikan. Status: poweroff"
      return 0
    fi
    sleep 2
    elapsed=$((elapsed + 2))
  done

  echo "Peringatan: VM tidak merespon ACPI shutdown dalam $timeout detik."
  echo "Kemungkinan penyebab: Guest Additions/acpid belum aktif, atau OS masih boot."
  exit 1
}

# create snapshot
create_snapshot() {
    VM_NAME="$1"
    SNAPSHOT_NAME="$2"
    print_header

    if [ -z "$VM_NAME" ] || [ -z "$SNAPSHOT_NAME" ]; then
        echo "Usage: ./vm_ctl4.sh snapshot create <nama_vm> <nama_snapshot>"
        exit 1
    fi

    echo "Membuat snapshot '$SNAPSHOT_NAME' pada VM '$VM_NAME'..."

    if VBoxManage snapshot "$VM_NAME" take "$SNAPSHOT_NAME" > /dev/null; then
        TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
        echo "Snapshot '$SNAPSHOT_NAME' berhasil dibuat pada $TIMESTAMP."
    else
        echo "Gagal membuat snapshot."
        exit 1
    fi
}

# list snapshot
list_snapshots() {
    VM_NAME="$1"

    if [ -z "$VM_NAME" ]; then
        echo "Usage: ./vm_ctl4.sh snapshot list <nama_vm>"
        exit 1
    fi

    echo "Daftar snapshot VM '$VM_NAME':"

    VBoxManage snapshot "$VM_NAME" list 2>&1 |
    sed -n 's/^[[:space:]]*Name:[[:space:]]*\(.*\)[[:space:]]*(UUID:.*)/\1/p' |
    sed 's/[[:space:]]*\*$//' |
    awk '{print "    " NR ". " $0}'
}

# snapshot command
snapshot() {
    case "$2" in
        create)
            create_snapshot "$3" "$4"
            ;;
        list)
            list_snapshots "$3"
            ;;
        *)
            echo "Perintah snapshot tidak dikenal."
            echo "Gunakan:"
            echo "./vm_ctl4.sh snapshot create <nama_vm> <nama_snapshot>"
            echo "./vm_ctl4.sh snapshot list <nama_vm>"
            exit 1
            ;;
    esac
}

case "$1" in
    list)
        list_vms
        ;;
    info)
        info "$@"
        ;;
    start)
        vm_start "$2"
        ;;
    snapshot)
        snapshot "$@"
        ;;
    stop)
        vm_stop "$2"
        ;;
    *)
        echo "Command tidak Dikenal. Gunakan:"
        echo "  $0 list"
        echo "  $0 info <nama_vm>"
        echo "  $0 start <nama_vm>"
        echo "  $0 stop <nama_vm>"
        echo "  $0 snapshot create <nama_vm> <nama_snapshot>"
        echo "  $0 snapshot list <nama_vm>"
        exit 1
        ;;
esac