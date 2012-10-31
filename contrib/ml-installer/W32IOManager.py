from IOManager import IOManager
import win32file
import win32api


class W32IOManager(IOManager):

    def is_removable(self, drive_num):
        mount_point = self.get_mount_point(drive_num)
        return win32file.GetDriveType(mount_point) == win32file.DRIVE_REMOVABLE

    def get_mount_point(self, drive_num):
        return r'%c:\\' % self.drive_letter(drive_num)

    def drive_letter(self, drive_num):
        return chr(ord('A') + drive_num)

    def enum_removable_drives(self):
            drivebits = win32file.GetLogicalDrives()
            return [d for d in range(1, 26) if (drivebits & (1 << d)) and self.is_removable(d)]

    def get_raw_device(self, drive_num):
        return r'\\.\%c:' % self.drive_letter(drive_num)

    def get_label(self, drive_num):
        mount_point = self.get_mount_point(drive_num)
        return win32api.GetVolumeInformation(mount_point)[0]

    def get_fs_type(self, drive_num):
        mount_point = self.get_mount_point(drive_num)
        return win32api.GetVolumeInformation(mount_point)[-1]
