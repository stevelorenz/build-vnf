//
// About: Server for slow-path.
//

fn main() {
    let ctx = zmq::Context::new();
    let sock = ctx.socket(zmq::REP).unwrap();
    assert!(sock.bind("ipc:///tmp/ffpp.sock").is_ok());

    let mut msg = zmq::Message::new();
    loop {
        sock.recv(&mut msg, 0).unwrap();
        println!("Received {}", msg.as_str().unwrap());
        sock.send("OK", 0).unwrap();
    }
}
