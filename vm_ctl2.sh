info() {
    echo "==================================="
    echo "     TUGAS 1 OS - KELOMPOK B08     "
    echo "==================================="	
    
    if VBoxManage list vms | awk -F'"' '{print $2}' | grep -Fxq "$2"; then
        vm = "$2"
        echo "VM Ditemukan: $vm"
        
                
    else
        echo "VM tidak ditemukan"
        return

    echo "Memindai RAM dan vCPU"
    memory = $(VBoxManage showvminfo "$2" --machinereadable | grep '^memory=' | cut -d'=' -f2)
    vcpu   = $(VBoxManage showvminfo "$2" --machinereadable | grep '^cpu=' | cut -d'=' -f2)

    echo "RAM: $memory"
    echo "vCPU: $vcpu"
}


case "$1" in
    list)
        list_vms
        ;;
    info)
        info
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
