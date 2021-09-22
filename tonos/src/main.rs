use clap::{crate_version, Arg};
use float_cmp::approx_eq;

fn main() {
    let args = clap::App::new("tonos")
        .version(crate_version!())
        .args(&[
            Arg::with_name("notes")
                .help("Notes per octave")
                .index(1)
                .default_value("12")
                .required(true),
            Arg::with_name("error")
                .help("Error treshold in cents")
                .short("e")
                .long("error")
                .default_value("100"),
            Arg::with_name("compare")
                .help("Compare temperaments")
                .short("c")
                .long("compare"),
            Arg::with_name("ratios")
                .help("Number of ratios to match")
                .short("r")
                .long("ratios")
                .default_value("1"),
            Arg::with_name("base")
                .help("Base")
                .short("b")
                .long("base")
                .default_value("2"),
            Arg::with_name("frequency")
                .help("Frequency multiplier")
                .short("f")
                .long("frequency")
                .default_value("1.0"),
            Arg::with_name("continuous")
                .help("Continuous comparison")
                .short("z")
                .long("continuous")
                .default_value("2"),
            Arg::with_name("density")
                .help("Density along the X axis (for continuous comparison)")
                .short("x")
                .long("density")
                .default_value("100"),
        ])
        .get_matches();
    let base: u32 = args.value_of("base").unwrap().parse().unwrap();
    let ratio_count: u32 = args.value_of("ratios").unwrap().parse().unwrap();
    if args.is_present("continuous") {
        let limit: u32 = args.value_of("continuous").unwrap().parse().unwrap();
        let density: u32 = args.value_of("density").unwrap().parse().unwrap();
        let mut ratios: Vec<(f64, f64)> = Vec::with_capacity(2 * ratio_count as usize);
        if ratio_count > 0 {
            let ratio_weight = |num, den, lcm| {
                let ratio = num as f64 / den as f64;
                let weight = (lcm as f64).recip();
                (ratio, weight)
            };
            for i in 2..ratio_count as u64 + 2 {
                ratios.push(ratio_weight(i, 1, i));
            }
            for den in 2_u64.. {
                let min_weight = ratios.last().unwrap().1;
                if ratio_weight(den + 1, den, (den + 1) * den).1 < min_weight {
                    break;
                }
                for num in den + 1.. {
                    let (gcd, lcm) = num_integer::gcd_lcm(num, den);
                    if gcd == 1 {
                        let (ratio, weight) = ratio_weight(num, den, lcm);
                        if weight < min_weight {
                            break;
                        }
                        let index = match ratios
                            .binary_search_by(|item| weight.partial_cmp(&item.1).unwrap())
                        {
                            Ok(index) => index,
                            Err(index) => index,
                        };
                        ratios.insert(index, (ratio, weight));
                    }
                }
                ratios.truncate(ratio_count as usize);
            }
        }
        let base_f64 = base as f64;
        println!("x,y");
        for i in density..density * limit + 1 {
            let x = i as f64 / density as f64;
            let t = base_f64.powf(x.recip());
            let (weighted_sum, total_weight) = ratios.iter().fold((0_f64, 0_f64), |acc, ratio| {
                let offset = ratio.0.log(t);
                let error = (offset - offset.round()).abs();
                let weight = ratio.1;
                (acc.0 + error * weight, acc.1 + weight)
            });
            println!("{},{}", x, 2.0 * weighted_sum / total_weight); // 2x to normalize error to [0, 1].
        }
    } else {
        let note_count: usize = args.value_of("notes").unwrap().parse().unwrap();
        if args.is_present("compare") {
            println!("notes,error_rel,error_abs");
            for i in 1..note_count + 1 {
                let error = Scale::new(base, i, ratio_count).total_error() * 100.0;
                println!("{},{},{}", i, error, error / i as f64);
            }
        } else {
            let frequency_multiplier: f64 = args.value_of("frequency").unwrap().parse().unwrap();
            let error_treshold: f64 = args.value_of("error").unwrap().parse().unwrap();
            let scale = Scale::new(base, note_count, ratio_count);
            for (i, note) in scale.notes.iter().enumerate() {
                print!(
                    "{:.16}f, // #{}",
                    note.frequency * frequency_multiplier,
                    i + 1
                );
                for ratio in &note.ratios {
                    let error = ratio.error * 100.0;
                    if error.abs() < error_treshold {
                        print!(" ({}:{}){:+.2}", ratio.num, ratio.den, error);
                    }
                }
                println!();
            }
            println!();
            println!("Total error: {}", scale.total_error() * 100.0);
        }
    }
}

struct Scale {
    notes: Vec<Note>,
}

struct Ratio {
    num: u32,
    den: u32,
    error: f64,
}

struct Note {
    offset: f64,
    frequency: f64,
    ratios: Vec<Ratio>,
}

impl Scale {
    fn new(base: u32, note_count: usize, ratio_count: u32) -> Scale {
        let base_f64 = base as f64;
        let frequency_step = (note_count as f64).recip();
        let mut notes: Vec<Note> = Vec::with_capacity(note_count);
        for i in 0..note_count + 1 {
            let offset = i as f64 * frequency_step;
            notes.push(Note::new(offset, base_f64.powf(offset)));
        }
        for (num, den, _) in Scale::generate_ratios(base, ratio_count) {
            let ratio_offset = (num as f64 / den as f64).log(base_f64);
            if let Some(min) = notes.iter_mut().min_by(|a, b| {
                (a.offset - ratio_offset)
                    .abs()
                    .partial_cmp(&(b.offset - ratio_offset).abs())
                    .unwrap()
            }) {
                let error = (min.offset - ratio_offset) * note_count as f64;
                debug_assert!(approx_eq!(f64, error.abs().max(0.5), 0.5, ulps = 48)); // At most half a note from the nearest note.
                min.ratios.push(Ratio { num, den, error });
            }
        }
        for note in &mut notes {
            note.ratios
                .sort_unstable_by(|a, b| a.error.abs().partial_cmp(&b.error.abs()).unwrap());
        }
        Scale { notes }
    }

    fn total_error(&self) -> f64 {
        let (weighted_sum, total_weight) = self.notes.iter().fold((0_f64, 0_f64), |a, note| {
            note.ratios.iter().fold(a, |a, ratio| {
                let weight = (num_integer::lcm(ratio.num, ratio.den) as f64).recip();
                (a.0 + 2.0 * ratio.error.abs() * weight, a.1 + weight)
            })
        });
        weighted_sum / total_weight
    }

    fn generate_ratios(base: u32, count: u32) -> Vec<(u32, u32, u32)> {
        assert!(base > 1);
        let mut ratios: Vec<(u32, u32, u32)> = Vec::with_capacity(count as usize);
        if count > 0 {
            for i in 1..base.min(count) + 1 {
                ratios.push((i, 1, i));
            }
            for den in 2.. {
                if ratios.len() == count as usize && ratios.last().unwrap().2 < den * (den + 1) {
                    break;
                }
                for num in den + 1..den * base {
                    let (gcd, lcm) = num_integer::gcd_lcm(num, den);
                    if gcd == 1 {
                        let index = match ratios
                            .binary_search_by(|item| item.2.cmp(&lcm).then(num.cmp(&item.1)))
                        {
                            Ok(index) => index + 1,
                            Err(index) => index,
                        };
                        ratios.insert(index, (num, den, lcm));
                    }
                }
                ratios.truncate(count as usize);
            }
        }
        ratios
    }
}

impl Note {
    fn new(offset: f64, frequency: f64) -> Note {
        Note {
            offset,
            frequency,
            ratios: Vec::new(),
        }
    }
}
