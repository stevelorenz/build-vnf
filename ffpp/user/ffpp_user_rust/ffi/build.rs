/*
 * Reference: https://github.com/capsule-rs/capsule
 * */

use std::env;
use std::path::{Path, PathBuf};

// TODO:  <14-08-20, Zuo> Reduce/Remove native DPDK libs //
#[cfg(not(feature = "rustdoc"))]
const RTE_CORE_LIBS: &[&str] = &[
    "rte_eal",
    "rte_mbuf",
    "rte_net",
    "rte_power",
    "rte_rcu",
    "rte_ring",
    "rte_timer",
];

#[cfg(not(feature = "rustdoc"))]
static RTE_PMD_LIBS: &[&str] = &["rte_pmd_af_packet"];

#[cfg(not(feature = "rustdoc"))]
const RTE_DEPS_LIBS: &[&str] = &["numa", "pcap"];

fn bind(path: &Path) {
    let bindings = bindgen::Builder::default()
        .header("./src/bindings.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate_comments(true)
        .generate_inline_functions(true)
        .clang_arg("-finline-functions")
        .generate()
        .expect("Unable to generate bindings!")
        .write_to_file(path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

pub fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bind(&out_path);

    // Link libraries dynamically.
    RTE_CORE_LIBS
        .iter()
        .chain(RTE_PMD_LIBS)
        .chain(RTE_DEPS_LIBS)
        .for_each(|lib| println!("cargo:rustc-link-lib=dylib={}", lib));

    // re-run build.rs upon changes.
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/");
}

#[cfg(features = "rustdoc")]
pub fn main() {}
