from IOManager import IOManager
import dbus


class LinuxIOManager(IOManager):

    def __init__(self):
        self.bus = dbus.SystemBus()
        self.hal_manager = self.bus.get_object("org.freedesktop.Hal", 
                                                            "/org/freedesktop/Hal/Manager")
        self.hal_iface = dbus.Interface(self.hal_manager, "org.freedesktop.Hal.Manager")

    def get_device_with_udi(self,  udi):
        obj = self.bus.get_object('org.freedesktop.Hal', udi)
        if obj != None:
            device = dbus.Interface(obj, 'org.freedesktop.Hal.Device')
            return device
        return None

    def get_mount_point(self, drive_num):
        return drive_num.GetProperty('volume.mount_point')

    def enum_removable_drives(self):
        udis = self.hal_iface.FindDeviceByCapability('volume')
        result = []
        for udi in udis:
            # get a hal object for the volume referenced by udi
            volume = self.get_device_with_udi(udi)
            parent_uri = volume.GetProperty('info.parent')
            parent = self.get_device_with_udi(parent_uri)
            if(parent.GetProperty('storage.removable')):
                result = result + [udi]
        return result

    def get_raw_device(self, drive_num):
	volume = self.get_device_with_udi(drive_num)
        return volume.GetProperty('block.device')

    def get_label(self, drive_num):
	volume = self.get_device_with_udi(drive_num)
        return volume.GetProperty('volume.label')

    def get_fs_type(self, drive_num):
	volume = self.get_device_with_udi(drive_num)
        return volume.GetProperty('volume.fsversion')
