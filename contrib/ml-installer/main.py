from LinuxIOManager import LinuxIOManager

io = LinuxIOManager()

d = io.enum_removable_drives()

for drive in d:
	print io.get_label(drive)
