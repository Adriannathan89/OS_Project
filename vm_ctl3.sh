#!/bin/bash

print_header() {
  echo "==================================="
  echo "TUGAS 1 OS - KELOMPOK AXX"
  echo "==================================="
}

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

# Dispatcher
case "$1" in
start)
  vm_start "$2"
  ;;
stop)
  vm_stop "$2"
  ;;
*)
  echo "Usage: $0 {start|stop} <nama_vm>"
  exit 1
  ;;
esac
