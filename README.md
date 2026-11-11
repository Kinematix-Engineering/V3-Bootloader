# V3 Bootloader 1.2

SAMD51P20A bootloader for KMX V3. The device exposes a USB CDC interface that
carries one UDS message per zero-delimited COBS frame. There is no UF2, mass
storage, screen, SAM-BA, or memory read service.

## Layout

| Flash address | Purpose |
| --- | --- |
| `0x0000–0x39FF` | Bootloader code |
| `0x3A00–0x3BFF` | Code-owned boot table: `KMX`, format `02`, 32-byte boot version, 64 reserved bytes, then `FF` padding |
| `0x3C00–0x3DFF` | Factory table, part page: 32-byte part, 3-byte hardware version, 1 reserved byte, 8-byte manufacturing date, then `FF` padding |
| `0x3E00–0x3FFF` | Factory table, security page: 16 × 16-byte secret keys, 2 × 32-byte public keys, then `FF` padding |
| `0x4000–0x7FFFF` | Application/program download range in Bank A |
| `0x80000–0xFFFFF` | Bank B; unavailable for program download |

The two shared layouts and fixed addresses are in
`lib/kmxcpp_v3_bootloader/v3_boot_table.h`. ASW can read them through
`v3_boot_table_get()` and `v3_factory_table_get()`. The boot table is defined
by the firmware image. The factory table is provisioned after manufacturing;
its unprovisioned bytes default to `FF`. The public key slots are reserved for
future signature validation. DID `F180` reports the boot version from the boot table.

The three pages share the bootloader's final 8192-byte erase block. End-of-line
provisioning can program the `FF` fields, but must not erase that block without
preserving its boot code and the other table pages.

The application-space `bootupdate-kmx_v3.bin` installs the new code-owned boot
table and always copies the installed factory table over the image defaults.
Blank devices retain their `FF` factory bytes. Afterward, the updater runs its erase
routine from RAM to remove its own application image and resets. Direct SWD flashing of
`bootloader-kmx_v3.bin` writes the default `FF` part/security pages. An
interrupted bootloader update requires SWD recovery.

## USB and UDS protocol

Each request and response is raw UDS bytes, COBS encoded and followed by `00`.
The maximum decoded message is 512 bytes. Integers in UDS requests are big
endian. The USB serial string and DID `F18C` use the same 16 serial bytes:
four SAMD51 unique-ID words at `008061FC`, `00806010`, `00806014`, `00806018`,
each transmitted least-significant byte first.

For an ASW-initiated switch into programming session, the ASW sends `7F 10 78`
in response to `10 02`, waits for that transfer to complete, calls
`v3_boot_request_set()`, then requests a system reset. The helper writes `B2`
to SRAM address `0x2003FFF8`, as defined in the shared header. The ASW
must reserve the top eight SRAM bytes so its stack does not overwrite the
request marker.
This byte is separate from the four-byte double-tap marker at
`HSRAM_ADDR + HSRAM_SIZE - 4`. The bootloader consumes the boot-request byte
only after a system reset, stays in bootloader mode, and sends one COBS-framed
`50 02` after the new USB CDC connection is configured and its data terminal
is open. The tester must reconnect to the new CDC device and wait for that
deferred response. Power-on or external resets clear the request byte without
sending `50 02`.

| Service | Request | Availability |
| --- | --- | --- |
| `22` | DID `F186` → current diagnostic session `02`; `F18C` → 16-byte serial; `F180` → 10-byte boot version | Before and after authentication |
| `27` | `01` → seed `01` + 16 random bytes; `02` + 16-byte response → unlock | Before authentication |
| `31` | `01 FF 00` + 4-byte address → erase one 8192-byte application block | Authenticated |
| `31` | `01 FF 01` + 4-byte length + 4-byte XOR + 4-byte sum → validate application bytes from `0x4000` | Authenticated, after download |
| `34` | `00 44` + 4-byte address + 4-byte size → start download | Authenticated |
| `36` | Sequence byte + up to 256 data bytes → write | Authenticated download |
| `37` | End download after exactly the requested size | Authenticated download |
| `11` | `01` → reset into application | Authenticated |

The `34` response advertises a maximum block length of 258 bytes including
the SID and sequence byte. Address and size must be 16-byte aligned and lie in
the `0x4000–0x7FFFF` application range in Bank A. Bank B, starting at
`0x80000`, is outside the programming range. Each covered 8192-byte block
must be erased first.
Transfer data length must be a nonzero multiple of 16. Sequence numbers start
at `01` and wrap at `FF`. A repeated final block with identical bytes is
accepted so a host can retry after losing its response. USB disconnect clears
authentication and download state. `35` (upload) and all other read-memory
services are unavailable.

Validation length is a nonzero multiple of four and must end within Bank A.
The bootloader treats each four bytes as a little-endian 32-bit word, XORs the
words, and sums them modulo 2³². Length and expected checksums are sent in
big-endian order. A mismatch returns NRC `72`. Validation currently changes
no application-valid marker.

The version-1 access response is `AES-128-ECB(fixed_key, seed[1..16]) XOR
chip_serial`. The same fixed key is compiled into the bootloader and the C#
host library, outside the reserved key table. This is a temporary access gate,
not a strong secret: anyone with the binaries can derive it. Five invalid keys
trigger a ten-second delay before another seed is issued.

## Build

Requires CMake, Ninja, Python 3, and `arm-none-eabi-gcc`. Initialize both
submodules with `git submodule update --init`, then run `make` in the repo
root. Outputs are `build/bootloader-kmx_v3.bin` (16 KiB),
`build/bootupdate-kmx_v3.bin`, and their ELF/map files. VS Code C/C++
IntelliSense uses `build/compile_commands.json` through
`.vscode/c_cpp_properties.json`.
GitHub Actions builds both firmware images and the .NET host projects and runs
protocol tests.

## Host CLI

Firmware sources and headers live under `src/flash`, `src/usb`, `src/uds`,
`src/crypto`, and `src/update`; startup, platform setup, and the main loop live directly
under `src/`. The CMSIS and SAMD51 device files live under `src/vendor`.

The `host/KMX.Flashing.UDS` C# class library implements identification,
authentication, erase, download, validation, and reset over a `Stream`. The minimal
`host/KMX.Flashing.Cli` wrapper opens a Linux USB CDC device; it requires .NET
10 and `stty`:

```sh
dotnet run --project host/KMX.Flashing.Cli -- identify /dev/ttyACM0
dotnet run --project host/KMX.Flashing.Cli -- flash /dev/ttyACM0 build/bootupdate-kmx_v3.bin
```

The device must already be in bootloader mode. `flash` expects an application
image linked for `0x4000`, pads the final 16-byte unit with `FF`, and requests
a reset after checksum validation. Use `bootupdate-kmx_v3.bin` to update the bootloader;
`bootloader-kmx_v3.bin` is the direct boot-flash image for SWD/KFlash.

## Packaging

KFlash generation is left disabled until `kdc` is available in the build
environment. `scripts/kflash.sh` records the legacy KFlash parameters and
the two planned packages. No `.kflash` is generated by the current build.

The protocol codec and UDS server come from the
[udslib submodule](https://github.com/nickdaria/udslib) at `lib/udslib`.
The [shared bootloader contract](https://github.com/Kinematix-Engineering/kmxcpp_v3_bootloader)
is the `lib/kmxcpp_v3_bootloader` submodule. Bootloader-specific handlers are in
`src/uds`.
