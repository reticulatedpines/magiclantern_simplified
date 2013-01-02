import os


class IOManager:

    def get_mount_point(self, drive_num):
        raise NotImplementedError("IOManager is an interface")

    def enum_removable_drives(self):
        raise NotImplementedError("IOManager is an interface")

    def get_raw_device(self, drive_num):
        raise NotImplementedError("IOManager is an interface")

    def get_label(self, drive_num):
        raise NotImplementedError("IOManager is an interface")

    def get_fs_type(self, drive_num):
        raise NotImplementedError("IOManager is an interface")

    def get_offset(self,  fs,  is_bootflag):
        offsets = {'FAT16':   [43,  64],
                        'FAT32':  [71, 92],
                        'EXFAT':  [130, 122]
                    }
        if(is_bootflag):
            return offsets[fs][1]
        else:
            return offsets[fs][0]

    def write_to_disk(self,  drive_num,  what,  sector=0, offset=0):
        raw_device = self.get_raw_device(drive_num)
        fd = open(raw_device,  'rb+')
        fd.seek(sector * 512,  os.SEEK_SET)
        olddata = fd.read(512)
        newdata = olddata[0:offset] + what + olddata[(offset + len(what)):]
        fd.seek(sector * 512,  os.SEEK_SET)
        fd.write(newdata)
        fd.close()

    def write_bootflag(self,  drive_num,  sector):
        fs = self.get_fs_type(drive_num)
        offset = self.get_offset(fs,  bootflag=True)
        self.write_to_disk(drive_num,  "BOOTDISK",  sector,  offset)

    def write_eosdevelop(self,  drive_num,  sector):
        fs = self.get_fs_type(drive_num)
        offset = self.get_offset(fs,  bootflag=False)
        self.write_to_disk(drive_num,  "EOS_DEVELOP",  sector,  offset)

    def write_vbr_checksum(self,  drive_num):
        pass

    def write_flags(self,  drive_num):
        fs = self.get_fs_type(drive_num)
        self.write_bootflag(drive_num,  0)
        self.write_eosdevelop(drive_num,  0)
        if(fs == "EXFAT"):
            self.write_bootflag(drive_num,  12)
            self.write_eosdevelop(drive_num,  12)
            self.write_vbr_checksum(drive_num)
