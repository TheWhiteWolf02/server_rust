mod server;
use crate::server::*;

fn main() {
    let server_thread = std::thread::spawn(|| {
        if let Err(err) = start_server() {
            eprintln!("[SERVER] Server error: {}", err);
        }
    });
    server_thread.join().expect("[SERVER] Server thread panicked");
}
