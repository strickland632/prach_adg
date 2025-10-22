if [ $EUID -ne 0 ]; then
        echo "run as root"
        exit 1
fi


docker run --privileged --net host -it -v /dev/bus/usb:/dev/bus/usb -v $(uhd_config_info --images-dir | awk '{print $3}'):/usr/share/uhd/images -v $1:/basic_prach.yaml srsran_gnb bash -c "prach-agent -c /basic_prach.yaml"
