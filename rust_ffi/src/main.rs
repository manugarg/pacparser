use ::pacparser::PacParser;
use clap::{crate_version, App, Arg, ArgMatches};
use std::fs::read_to_string;

fn main() {
    let matches = parse_arguments();

    let file = read_to_string(matches.value_of("pac_file").unwrap()).unwrap();
    let url = matches.value_of("url").unwrap();
    let p = PacParser::new(&file).unwrap();
    let r = p.find_proxy(url).unwrap();
    print!("{}", r);
}

pub fn parse_arguments() -> ArgMatches {
    App::new("pacparser")
        .about("find proxy based on a pac file.")
        .version(crate_version!())
        .arg(
            Arg::with_name("pac_file")
                .short('p')
                .long("pac-file")
                .takes_value(true)
                .required(true)
                .about("path to pac file."),
        )
        .arg(
            Arg::with_name("url")
                .short('u')
                .long("url")
                .takes_value(true)
                .required(true)
                .about("url for which to find the proxy."),
        )
        .get_matches()
}
