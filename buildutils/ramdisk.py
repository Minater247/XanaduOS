# This script generates a ramdisk image from the ./ramdisk directory. Presently, the ramdisk
# is an uncompressed custom format.

#The format of a ramdisk is as follows:
#- The first 4 bytes are the number of files in the ramdisk (uint32)
#- The next 4 bytes are the length of the file list (uint32)
#- A list of file entries or directory entries

#A file entry is as follows:
#  - 2 bytes: a magic number declaring this is a file entry (uint16)
#  - 4 bytes: the offset of the file in the ramdisk (uint32)
#  - 64 bytes: the name of the file (null terminated)
#  - 4 bytes: the length of the file (uint32)

#A directory entry is as follows:
#  - 2 bytes: a magic number declaring this is a directory entry (uint16)
#  - 64 bytes: the name of the directory (null terminated)
#  - 4 bytes: the number of files in the directory (uint32)
#  - The file and directory entries for the directory
#  - 2 bytes: a magic number declaring the end of the directory (uint16)

#The ramdisk is then padded to a multiple of 512 bytes with 0s.

#The program should ask for the output file name and the ramdisk directory. If
#the ramdisk directory is not specified, it should default to ./ramdisk.

import os
import struct
import sys

#Magic numbers
FILE_ENTRY = 0xBAE7
DIR_ENTRY = 0x7EAB
DIR_EXIT = 0x7EAC

def main():
    #Get the output file name
    if len(sys.argv) < 3:
        print("Usage: ramdisk.py output [ramdiskDir]")
        return
    if len(sys.argv) > 2:
        output = sys.argv[1]
        ramdiskDir = sys.argv[2]
    else:
        ramdiskDir = "./ramdisk"

    print("Ramdisk dir:")
    print(ramdiskDir)

    file_tree = []
    def get_file_tree(path):
        tree = []
        for file in os.listdir(path):
            if os.path.isdir(path + "/" + file):
                tree.append([file, get_file_tree(path + "/" + file)])
            else:
                tree.append(file)
        return tree

    file_tree = get_file_tree(ramdiskDir)

    def print_tree(tree, indent):
        for file in tree:
            if type(file) is list:
                print(indent + file[0])
                print_tree(file[1], indent + "    ")
            else:
                print(indent + file)

    print("")
    print_tree(file_tree, "")

    #Open the output file
    f = open(output, "wb")

    #Write the header
    num_files = 0
    def count_files(tree):
        count = 0
        for file in tree:
            if type(file) is list:
                count += count_files(file[1])
            else:
                count += 1
        return count
    
    num_files = count_files(file_tree)
    f.write(struct.pack("<I", num_files))

    #Create a second buffer to write the file list to
    file_list = bytearray()

    #Write the file list
    def write_file_list_old(tree, offset):
        for file in tree:
            if type(file) is list:
                #Write the directory entry
                file_list.extend(struct.pack("<H", DIR_ENTRY))
                file_list.extend(file[0].encode("utf-8"))
                file_list.extend(b"\0" * (64 - len(file[0])))
                file_list.extend(struct.pack("<I", count_files(file[1])))
                offset = write_file_list(file[1], offset)
                file_list.extend(struct.pack("<H", DIR_EXIT))
            else:
                #Write the file entry
                file_list.extend(struct.pack("<H", FILE_ENTRY))
                file_list.extend(struct.pack("<I", offset))
                file_list.extend(file.encode("utf-8"))
                file_list.extend(b"\0" * (64 - len(file)))
                file_size = os.path.getsize(ramdiskDir + "/" + file)
                file_list.extend(struct.pack("<I", file_size))
                offset += file_size
        return offset

    #Better write_file_list, that takes into account what the parent directory/ies are
    #when opening a file (because for something like "./ramdisk/nested/file.txt", the file
    #is actually opened as ".ramdisk/file.txt", not "./ramdisk/nested/file.txt")
    #This should probably be rewritten to take a path argument as well
    def write_file_list(tree, offset, path):
        for file in tree:
            if type(file) is list:
                #Write the directory entry
                file_list.extend(struct.pack("<H", DIR_ENTRY))
                file_list.extend(file[0].encode("utf-8"))
                file_list.extend(b"\0" * (64 - len(file[0])))
                file_list.extend(struct.pack("<I", count_files(file[1])))
                offset = write_file_list(file[1], offset, path + "/" + file[0])
                file_list.extend(struct.pack("<H", DIR_EXIT))
                #throw an error if the file name contains a magic number
                if "\xBA\xE7" in file[0]:
                    print("Error: directory name contains magic number")
                    return
                if "\x7E\xAB" in file[0]:
                    print("Error: directory name contains magic number")
                    return
                if "\x7E\xAC" in file[0]:
                    print("Error: directory name contains magic number")
                    return
            else:
                #Write the file entry
                file_list.extend(struct.pack("<H", FILE_ENTRY))
                file_list.extend(struct.pack("<I", offset))
                file_list.extend(file.encode("utf-8"))
                file_list.extend(b"\0" * (64 - len(file)))
                file_size = os.path.getsize(ramdiskDir + path + "/" + file)
                file_list.extend(struct.pack("<I", file_size))
                offset += file_size
                #throw an error if the file name contains a magic number
                if "\xBA\xE7" in file:
                    print("Error: file name contains magic number")
                    return
                if "\x7E\xAB" in file:
                    print("Error: file name contains magic number")
                    return
                if "\x7E\xAC" in file:
                    print("Error: file name contains magic number")
                    return
        return offset

    write_file_list(file_tree, 0, "")

    #Write the file list length
    f.write(struct.pack("<I", len(file_list)))

    #Write the file list
    f.write(file_list)

    #Write the files
    def write_files(tree, path):
        for file in tree:
            if type(file) is list:
                write_files(file[1], path + "/" + file[0])
            else:
                f.write(open(ramdiskDir + path + "/" + file, "rb").read())

    write_files(file_tree, "")

    #Pad the file to a multiple of 512 bytes
    f.write(b"\0" * (512 - (f.tell() % 512)))

    #Close the file
    f.close()


if __name__ == "__main__":
    main()