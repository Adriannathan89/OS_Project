info() {
    echo "==================================="
    echo "     TUGAS 1 OS - KELOMPOK B08     "
    echo "==================================="	
    
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


case "$1" in
    list)
        list_vms
        ;;
    info)
        info "$@"
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
