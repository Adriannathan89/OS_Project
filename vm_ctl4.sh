#!/bin/bash

echo "======================================"
echo "      TUGAS 1 OS - KELOMPOK AXX"
echo "======================================"


# create snapshot
create_snapshot() {
    VM_NAME="$1"
    SNAPSHOT_NAME="$2"

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
        ;;
    start)
        ;;
    snapshot)
        snapshot "$@"
        ;;
    stop)
        ;;
    *)
        echo "Usage: $0 list"
        exit 1
        ;;
esac