use std::env;
use std::io::{self, Error, BufRead};
use prost::Message;
use chrono::{DateTime, TimeZone, Utc, Local};
use prost::encoding::uint64;
use reqwest;

use axum::{
    response::Json,
    routing::get,
    Router
};
use serde::{Serialize, Deserialize};
use serde_json::{Value, json};

use tokio;

extern crate chrono;

pub mod transit_realtime {
     include!(concat!(env!("OUT_DIR"), "/transit_realtime.rs"));
}

pub mod mta {
    include!(concat!(env!("OUT_DIR"), "/stops.rs"));
}

#[derive(Serialize, Deserialize)]
struct Entry {
    line: String,
    direction: String,
    leaving_at_timestamp: i64,
}

fn handle_transit_rt(bytes: &[u8]) -> Vec<Entry> {
    let mut res = vec![];

    let msg  = transit_realtime::FeedMessage::decode(bytes).unwrap();

    let ts = Utc.timestamp(msg.header.timestamp.unwrap_or(0).try_into().unwrap(), 0);
    println!(
        "gtfs_realtime_version: {}, timestamp {}", 
        msg.header.gtfs_realtime_version, ts);

    for entity in msg.entity {
        // println!("entity id: {}, deleted={}", entity.id, entity.is_deleted.unwrap_or(false));
        // println!("entity is TripUpdate={}, entity is VehiclePosition={}, entity is Alert={}", entity.trip_update.is_some(), entity.vehicle.is_some(), entity.alert.is_some());

        if let Some(trip_update) = &entity.trip_update {
            let route_id = (&trip_update.trip).route_id.as_ref().map_or("Unlnown", |x| x.as_str());
            //if !route_id.eq("1") {
            //    continue;
            //}

            // println!("Trip: {}. There is {} train to {} ", 
            // trip_update.trip.trip_id.unwrap_or("Unlnown".to_string()),
            // trip_update.trip.route_id.unwrap_or("Unlnown".to_string()),
            // trip_update.trip.direction_id.unwrap_or(0));

            for stop in &trip_update.stop_time_update {
                let stop_id = stop.stop_id.as_ref().map(|x| x.as_str());
                let stop_name = stop_id.map_or(None, |x| mta::stop_name(x));

                if !stop_name.unwrap_or("").eq("116 St-Columbia University") {
                    continue;
                }

                let t: DateTime<Local> = Utc.timestamp(stop.departure.as_ref().map(|a| a.time.unwrap_or(0)).unwrap_or(0), 0).into();

                
                println!("Train: {}, stop: {} or {} at time: {}, delay: {}", 
                    route_id,
                    stop_id.unwrap_or("??"),
                    stop_name.unwrap_or("??"),
                    t,
                    stop.arrival.as_ref().map(|a| a.delay.unwrap_or(0)).unwrap_or(0)
                );

                let entry = Entry{// format!("Train: {}, stop: {} or {} at time: {}, delay: {}", 
                    line: route_id.to_owned(),
                    direction: stop_id.unwrap_or("??").to_owned(),
                    leaving_at_timestamp: t.timestamp_millis()
                };
                
                res.push(entry);
            }
            
        }
    }

    res
}

async fn next_train() -> Result<Vec<Entry>, Box<reqwest::Error>> {
    let mut headers = reqwest::header::HeaderMap::new();
    let mut api_key = reqwest::header::HeaderValue::from_str(&env::var("API_KEY").unwrap()).unwrap();
    api_key.set_sensitive(true);


    headers.insert(
        "x-api-key",
        api_key
    );

    // get a client builder
    let client = reqwest::Client::builder()
        .default_headers(headers)
        .build().unwrap();

    let res = client
        .get("https://api-endpoint.mta.info/Dataservice/mtagtfsfeeds/nyct%2Fgtfs")
        .send()
        .await?;
    
    let content = res.bytes().await?;

    Ok(handle_transit_rt(&content))
}

#[tokio::main]
async fn main() {
    // build our application with a single route
    let app = Router::new()
        .route("/", get(|| async { "Hello, World!" }))
        .route("/next_train", get(|| async { 
            let res = next_train().await;

            match res {
                Ok(entries) => Json(serde_json::to_value(entries).unwrap()),
                Err(_) => Json(json!({ "data": 42 }))
            }
        }))
    ;

    // run it with hyper on localhost:3000
    axum::Server::bind(&"0.0.0.0:3000".parse().unwrap())
        .serve(app.into_make_service())
        .await
        .unwrap();
}
