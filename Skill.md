# Skill: แก้ปัญหา Pico SDK Build พัง เมื่อติดตั้ง STM32CubeCLT ร่วมกัน

## อาการ

- `ninja: error: loading 'build.ninja': The system cannot find the file specified.`
- `fatal error: dsp/support_functions.h: No such file or directory`
- `fatal error: arm_math_memory.h: No such file or directory`
- Build ผ่านครั้งแรก แต่พังหลังปิดเปิด VS Code ใหม่

## สาเหตุ

STM32CubeCLT ติดตั้ง cmake ของตัวเองไว้ใน **System PATH** ก่อน cmake ของ Pico SDK:

```
C:\ST\STM32CubeCLT_1.21.0\CMake\bin\cmake.exe  ← ผิดตัว (อยู่ก่อนใน PATH)
C:\Users\<user>\.pico-sdk\cmake\v3.31.5\bin\cmake.exe  ← ถูกตัว
```

เมื่อ ninja ต้องการ re-run cmake ระหว่าง build มันเรียก cmake ตาม PATH ของระบบ → ได้ cmake ของ STM32 → configure พัง → CMSIS-DSP source โดน partial delete → headers หาย

## วิธีแก้ถาวร (เฉพาะโปรเจค)

แก้ไฟล์ `.vscode/tasks.json` — เปลี่ยน task "Compile Project" ให้เรียก cmake ของ Pico โดยตรงแทน ninja:

```json
{
    "label": "Compile Project",
    "type": "process",
    "isBuildCommand": true,
    "command": "${env:USERPROFILE}/.pico-sdk/cmake/v3.31.5/bin/cmake.exe",
    "args": ["--build", "${workspaceFolder}/build"],
    "options": {
        "env": {
            "PATH": "${env:USERPROFILE}/.pico-sdk/cmake/v3.31.5/bin;${env:USERPROFILE}/.pico-sdk/ninja/v1.12.1;${env:USERPROFILE}/.pico-sdk/toolchain/15_2_Rel1/bin;${env:PATH}"
        }
    },
    "group": "build",
    "presentation": {
        "reveal": "always",
        "panel": "dedicated"
    },
    "problemMatcher": "$gcc"
}
```

> เปลี่ยน `v3.31.5` และ `15_2_Rel1` ให้ตรงกับ version ที่ติดตั้งในเครื่องด้วย

## วิธีแก้ชั่วคราว (ถ้า build folder พัง)

รันใน PowerShell:

```powershell
# 1. ลบ build folder
Remove-Item -Recurse -Force ".\build"
New-Item -ItemType Directory ".\build" | Out-Null

# 2. Configure ด้วย cmake ถูกตัว
$env:PATH = "C:\Users\$env:USERNAME\.pico-sdk\cmake\v3.31.5\bin;" + ($env:PATH -replace "C:\\ST\\STM32CubeCLT_1\.21\.0\\CMake\\bin;", "")
cd build
cmake .. -G Ninja

# 3. Build
cmake --build .
```

## หมายเหตุ

- Pico SDK มี Python bundled อยู่แล้วที่ `~/.pico-sdk/python/` ไม่ต้องติดตั้ง Python แยก
- ถ้าลบ build folder แล้ว VS Code จะ auto-configure ผ่าน `raspberry-pi-pico.cmakeAutoConfigure: true` ซึ่งใช้ cmake ถูกตัวอยู่แล้ว (ตาม `cmake.cmakePath` ใน settings.json)
- ปัญหานี้เกิดเฉพาะเมื่อมี STM32CubeCLT ติดตั้งร่วมกับ Pico SDK เท่านั้น
