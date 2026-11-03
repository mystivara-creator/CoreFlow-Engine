fn main() {
    cxx_build::bridge("src/bridge.rs")
        .std("c++23")
        .flag_if_supported("-fno-exceptions")
        .compile("coreflow-bridge");
    println!("cargo:rerun-if-changed=src/bridge.rs");
    println!("cargo:rerun-if-changed=src/controller.rs");
}
