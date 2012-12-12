#!/bin/bash

echo "############################################################"
echo "############################################################"
echo "     Insert customized pcan driver into the kernel tree     "
echo "############################################################"
echo "############################################################"

CURRENT_DIRECTORY=`pwd`
echo "${CURRENT_DIRECTORY}"

if [ `echo "${CURRENT_DIRECTORY}" | grep -e linux-dev$` ] ; then
	echo "step out of linux-dev directory"
	cd ..
	CURRENT_DIRECTORY=`pwd`
	echo "${CURRENT_DIRECTORY}"
else
	if [ `echo "${CURRENT_DIRECTORY}" | grep -e LemonAid$` ] ; then
		echo "correct directory"
	fi
fi

PCAN_DRIVER_EXIST=`ls ./linux-dev/KERNEL/drivers/misc | grep -e pcan`
if [ "${PCAN_DRIVER_EXIST}" ] ; then
	echo "driver exists!"
else
	echo "copying drivers to linux kernel"
	mkdir ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/pcan

	cp ${CURRENT_DIRECTORY}/pcandriver/* ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/pcan/
	echo "obj-\$(CONFIG_PCAN_LEMONAID) += pcan/" >> ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/Makefile
	cp ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/Kconfig ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/Kconfig1
	sed '/carma/asource "drivers\/misc\/pcan\/Kconfig"' ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/Kconfig1 > ${CURRENT_DIRECTORY}/linux-dev/KERNEL/drivers/misc/Kconfig

	cd ${CURRENT_DIRECTORY}/linux-dev/KERNEL/
	git add .
	git commit -am "add pcan driver"
	cd ..
	git add .
	git commit -am "update git script"
fi
