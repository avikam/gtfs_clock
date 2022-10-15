use std::fs::File;
use std::ops::Deref;
use std::io::{self, Result, BufRead, Write};
use std::collections::HashMap;
use std::{fs, env};
use std::path::Path;

fn generate_stops_map() -> Result<()> {
    let mut filtered = HashMap::new(); 
    
    let f = fs::File::open("openmobilitydata-data-mta-20220615-stops.txt")?;
    let mut lines = io::BufReader::new(f).lines();

    let header_line = lines.next().unwrap().unwrap();
    let headers: Vec<_> = header_line.split(",").collect();

    for line in lines {
        let entry: Vec<_> = line.as_ref().unwrap().split(",").collect();
        let hashed_entry: HashMap<&&str, &&str> = headers.iter().zip(entry.iter()).collect();

        if hashed_entry.get(&"stop_name").unwrap().deref().eq(&"116 St-Columbia University") || true {
            filtered.insert(hashed_entry.get(&"stop_id").unwrap().to_string(), hashed_entry.get(&"stop_name").unwrap().to_string());
        }
    }

    let out_dir = env::var_os("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("stops.rs");

    let mut buffer = fs::File::create(dest_path)?;
    buffer.write_all(b"
use std::collections::HashMap;
use lazy_static::lazy_static;

lazy_static! {
    static ref HASHMAP: HashMap<&'static str, &'static str> = {
        let mut m = HashMap::new();
")?;

    for (stop_id, stop_name) in filtered.iter() {
        let x = format!("m.insert(\"{}\", \"{}\"); ", stop_id, stop_name);
        buffer.write_all(x.as_bytes())?;
    };

    buffer.write_all(b"
    m }; }
    pub fn stop_name(stop_id: &str) -> Option<&'static str> {
        HASHMAP.get(stop_id).map(|r| *r)
    }
")?;
    Ok(())
}

fn main() -> Result<()> {
    generate_stops_map()?;

    prost_build::compile_protos(&["src/gtfs-realtime.proto"], &["src/"])?;

    Ok(())
}
