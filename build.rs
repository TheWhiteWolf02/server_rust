fn main() {
    let src = [
        "src/mmc.c",
        "src/mmc_cmds.c",
    ];
    let mut builder = cc::Build::new();
    let build = builder
        .files(src.iter())
        .include("include")
        .flag("-Wno-unused-parameter")
        .define("USE_ZLIB", None);
    build.compile("foo");
}