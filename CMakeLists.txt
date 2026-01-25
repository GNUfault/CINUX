cmake_minimum_required(VERSION 3.10)
project(CINUX C ASM)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-f elf32")
enable_language(ASM_NASM)

set(BUILD_DIR ${CMAKE_BINARY_DIR}/out)
set(ISO_DIR ${CMAKE_BINARY_DIR}/iso)
set(DISK_IMG ${BUILD_DIR}/disk.img)
set(ISO_IMG ${BUILD_DIR}/QuantumKernel.iso)

set(CFLAGS -std=c99 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -I${CMAKE_SOURCE_DIR}/include -m32)
set(ASFLAGS -f elf32)
set(LDFLAGS -m elf_i386)

set(SRC_DIRS
    src/kernel src/cpu src/drivers src/mm src/user src/syscall src/lib src/fs src/errors
    src/drivers/pci src/drivers/rtl8139 src/drivers/net
)

file(GLOB_RECURSE ASM_SOURCES 
    src/kernel/*.asm src/cpu/*.asm src/drivers/*.asm src/mm/*.asm 
    src/user/*.asm src/syscall/*.asm src/lib/*.asm src/fs/*.asm 
    src/errors/*.asm src/drivers/pci/*.asm src/drivers/rtl8139/*.asm 
    src/drivers/net/*.asm
)

file(GLOB_RECURSE C_SOURCES 
    src/kernel/*.c src/cpu/*.c src/drivers/*.c src/mm/*.c 
    src/user/*.c src/syscall/*.c src/lib/*.c src/fs/*.c 
    src/errors/*.c src/drivers/pci/*.c src/drivers/rtl8139/*.c 
    src/drivers/net/*.c
)

set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM_NASM)

add_executable(kernel.elf ${ASM_SOURCES} ${C_SOURCES})
target_compile_options(kernel.elf PRIVATE 
    $<$<COMPILE_LANGUAGE:C>:${CFLAGS}>
)
set_target_properties(kernel.elf PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
    SUFFIX ""
    LINK_DEPENDS ${CMAKE_SOURCE_DIR}/src/kernel/link.ld
)
set_target_properties(kernel.elf PROPERTIES LINK_FLAGS "-m32 -nostdlib")
target_link_options(kernel.elf PRIVATE 
    "LINKER:-m,elf_i386"
    "LINKER:-T,${CMAKE_SOURCE_DIR}/src/kernel/link.ld"
)

if(EXISTS ${CMAKE_SOURCE_DIR}/userspace/libc)
    file(GLOB LIBC_SOURCES ${CMAKE_SOURCE_DIR}/userspace/libc/src/*.c)
    
    set(LIBC_OBJS "")
    foreach(src ${LIBC_SOURCES})
        get_filename_component(name ${src} NAME_WE)
        set(obj ${BUILD_DIR}/libc_${name}.o)
        
        add_custom_command(
            OUTPUT ${obj}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
            COMMAND gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-builtin 
                    -I${CMAKE_SOURCE_DIR}/userspace/libc/include -c ${src} -o ${obj}
            DEPENDS ${src}
            COMMENT "Building libc object ${name}.o"
        )
        
        list(APPEND LIBC_OBJS ${obj})
    endforeach()
    
    add_custom_command(
        OUTPUT ${BUILD_DIR}/libc.a
        COMMAND ar rcs ${BUILD_DIR}/libc.a ${LIBC_OBJS}
        DEPENDS ${LIBC_OBJS}
        COMMENT "Creating libc.a"
    )
    
    add_custom_target(libc ALL DEPENDS ${BUILD_DIR}/libc.a)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/userspace)
    file(GLOB USERSPACE_PROGS ${CMAKE_SOURCE_DIR}/userspace/*.c)
    
    foreach(prog_src ${USERSPACE_PROGS})
        get_filename_component(name ${prog_src} NAME_WE)
        
        add_custom_command(
            OUTPUT ${BUILD_DIR}/${name}.elf
            COMMAND gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-builtin 
                    -I${CMAKE_SOURCE_DIR}/userspace/libc/include -c ${prog_src} 
                    -o ${BUILD_DIR}/${name}.o
            COMMAND ld -m elf_i386 -T ${CMAKE_SOURCE_DIR}/userspace/link.ld 
                    ${BUILD_DIR}/${name}.o ${BUILD_DIR}/libc.a -o ${BUILD_DIR}/${name}.elf
            COMMAND ${CMAKE_COMMAND} -E remove ${BUILD_DIR}/${name}.o
            DEPENDS ${prog_src} ${BUILD_DIR}/libc.a ${CMAKE_SOURCE_DIR}/userspace/link.ld
            COMMENT "Building userspace program ${name}.elf"
        )
        
        add_custom_target(${name} ALL DEPENDS ${BUILD_DIR}/${name}.elf)
    endforeach()
endif()

add_custom_command(
    OUTPUT ${DISK_IMG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}
    COMMAND dd if=/dev/zero of=${DISK_IMG} bs=1M count=16 2>/dev/null
    COMMAND mkfs.fat -F 32 ${DISK_IMG}
    COMMENT "Creating FAT32 disk image"
)

add_custom_target(disk ALL DEPENDS ${DISK_IMG})

add_custom_command(
    TARGET disk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "UTC+3" >${BUILD_DIR}/tz.txt
    COMMAND mcopy -i ${DISK_IMG} -o ${BUILD_DIR}/tz.txt ::tz.txt
    COMMAND ${CMAKE_COMMAND} -E echo "Type help to view commands, thanks for download \\<3" >${BUILD_DIR}/help.txt
    COMMAND mcopy -i ${DISK_IMG} -o ${BUILD_DIR}/help.txt ::help.txt
    COMMAND test -f ${BUILD_DIR}/hello.elf && mcopy -i ${DISK_IMG} -o ${BUILD_DIR}/hello.elf ::hello.elf || true
    COMMENT "Writing files to disk image"
)

file(WRITE ${CMAKE_BINARY_DIR}/write_grub_cfg.cmake "
file(WRITE \"\${GRUB_CFG}\" \"set timeout=0
set default=0

menuentry \\\"QuantumKernel\\\" {
    multiboot /boot/kernel.elf
    set gfxpayload=1024x768x32
    boot
}
\")
")

add_custom_command(
    OUTPUT ${ISO_IMG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_DIR}/boot/grub
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_DIR}/kernel.elf ${ISO_DIR}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -DGRUB_CFG=${ISO_DIR}/boot/grub/grub.cfg -P ${CMAKE_BINARY_DIR}/write_grub_cfg.cmake
    COMMAND grub-mkrescue -o ${ISO_IMG} ${ISO_DIR} 2>/dev/null || grub2-mkrescue -o ${ISO_IMG} ${ISO_DIR} 2>/dev/null
    DEPENDS ${BUILD_DIR}/kernel.elf
    COMMENT "Creating bootable ISO"
)

add_custom_target(iso ALL DEPENDS ${ISO_IMG})

add_custom_target(run
    COMMAND qemu-system-x86_64
        -vga std
        -boot d
        -cdrom ${ISO_IMG}
        -hda ${DISK_IMG}
        -m 128M
        -netdev user,id=net0
        -device rtl8139,netdev=net0
        -no-reboot
        -no-shutdown
        -rtc base=localtime,clock=host
    DEPENDS iso disk
    COMMENT "Running QuantumKernel in QEMU"
)
