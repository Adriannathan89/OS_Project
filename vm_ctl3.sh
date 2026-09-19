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