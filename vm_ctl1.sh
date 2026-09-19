#!/bin/bash

list_vms() {
    echo "==================================="
    echo "     TUGAS 1 OS - KELOMPOK B08     "
    echo "==================================="

    echo " Memindai daftar Virtual Machine..."

    echo " Daftar Virtual Machine:"
    VboxManage list vms | awk -F'"' '{print $2}' | while read vm; do
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