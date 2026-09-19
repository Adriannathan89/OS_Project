#!/bin/bash

list_vms() {
    echo "==================================="
    echo "     TUGAS 1 OS - KELOMPOK B08     "
    echo "==================================="

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

case "$1" in
    list)
        list_vms
        ;;
    info)
        ;;
    start)
        ;;
    snapshot)
        ;;
    stop)
        ;;
    *)
        echo "Usage: $0 list"
        exit 1
        ;;
esac