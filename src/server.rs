use std::{ffi::CString, net::UdpSocket, os::raw::c_char};

const EMMCPORT: u16 = 24000;
const MAX_UINT64_SIZE: usize = 21;
const READEB_CMD: &str = "READ_EB";
// const KEY_VALUE: &str = "YRi55\\GAxgEZD9viP>j8=IUe;oIjYPY\n";

extern "C" {
    fn read_block() -> *mut c_char;
    fn write_block(input: *mut c_char) -> std::os::raw::c_int;
}

pub fn start_server() -> std::io::Result<()> {
    let server_addr = format!("192.168.2.2:{}", EMMCPORT);
    let socket = UdpSocket::bind(&server_addr)?;

    loop {
        println!("[SERVER] hello");
        let mut buf = [0u8; MAX_UINT64_SIZE];
        let (_, client_addr) = socket.recv_from(&mut buf)?;

        let received_cmd = std::str::from_utf8(&buf)
            .expect("[SERVER] Invalid UTF-8")
            .trim()
            .trim_end_matches('\0');

        println!("[SERVER] received_cmd after parsing {}", received_cmd);

        if received_cmd.contains(READEB_CMD) {
            let read_value: *mut c_char = unsafe { read_block() };
            let c_str = unsafe { CString::from_raw(read_value) };
            let rust_str = c_str.to_str().unwrap();
            println!("[SERVER] value read from memory {}", rust_str);
            socket.send_to(rust_str.as_bytes(), client_addr)?;
        } else {
            let input_cstring = CString::new(received_cmd).expect("[SERVER] CString conversion failed");
            let result = unsafe { write_block(input_cstring.as_ptr() as *mut u8) };

            if result == 0 {
                println!("[SERVER] Writing new value {} successful", received_cmd);
            } else {
                println!("[SERVER] Writing new value {} failed", received_cmd);
            }
        }
    }
}

/*
[build]
target = "x86_64-unknown-linux-musl"

[target.'cfg(target_os = "linux")']
rustflags = ["-C", "linker=ld.lld", "-C", "relocation-model=static", "-C", "strip=symbols"]

fn write_to_rpmb(value: u64) {
    // Convert the u64 variable to a byte array of 256 bytes
    let value_bytes: [u8; 8] = value.to_le_bytes();
    let mut data = vec![0u8; 256];
    data[0..8].copy_from_slice(&value_bytes);

    // Create a file and write the byte array to it
    let mut file = File::create("data").expect("[SERVER] Failed to create file");
    file.write_all(&data)
        .expect("[SERVER] Failed to write to file");

    // Write the key to a file named "key"
    let mut file = File::create("key").expect("[SERVER] Failed to create file");
    file.write_all(KEY_VALUE.as_bytes())
        .expect("[SERVER] Failed to write to file");

    // Write the key to RPMB
    let mmc_command = Command::new("sudo")
        .arg("mmc")
        .args(&[
            "rpmb",
            "write-block",
            "/dev/mmcblk1rpmb",
            "0",
            "data",
            "key",
        ])
        .status()
        .expect("[SERVER] Failed to execute mmc rpmb write-key");

    if mmc_command.success() {
        println!("[SERVER] Key successfully written to RPMB");
    } else {
        println!("[SERVER] Failed to write key to RPMB");
    }
}

fn read_from_rpmb() -> u64 {
    let mut file = File::create("key").expect("[SERVER] Failed to create file");
    file.write_all(KEY_VALUE.as_bytes())
        .expect("[SERVER] Failed to write to file");

    // Read the key from RPMB
    let read_command = Command::new("sudo")
        .arg("mmc")
        .args(&[
            "rpmb",
            "read-block",
            "/dev/mmcblk1rpmb",
            "0",
            "1",
            "-",
            "key",
        ])
        .output()
        .expect("[SERVER] Failed to execute mmc rpmb read-key");

    if read_command.status.success() {
        let key_bytes = &read_command.stdout;

        if key_bytes.len() >= 8 {
            let mut key_u64_bytes = [0u8; 8];
            key_u64_bytes.copy_from_slice(&key_bytes[..8]);
            let key_u64 = u64::from_le_bytes(key_u64_bytes);
            println!(
                "[SERVER] Key successfully read from RPMB as u64: {}",
                key_u64
            );
            return key_u64;
        } else {
            println!(
                "[SERVER] Read key from RPMB, but data length is insufficient to parse as u64"
            );
        }
    }

    println!("[SERVER] Failed to read key from RPMB");
    return 0;
}
*/
