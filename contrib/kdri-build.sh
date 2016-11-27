# rebuild
rm -Rf kdri-c-wrapper
git clone https://github.com/ChangSpivey/kdri-c-wrapper
cd kdri-c-wrapper
cargo build --release # compile static and dynamic library
