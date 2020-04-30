use bindgen;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=native=./libs");
    println!("cargo:rustc-link-lib=pacparser");
    let bindings = bindgen::Builder::default()
        .header("includes.h")
        .generate()
        .unwrap();

    let out_path = PathBuf::from("./src");
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .unwrap();
}
