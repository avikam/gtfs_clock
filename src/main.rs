use std::borrow::BorrowMut;
use std::collections::HashMap;
use std::{env, default};
use std::sync::Arc;
use std::io::{self, Error, BufRead};
use std::time::Duration;


use prost::Message;
use chrono::{DateTime, TimeZone, Utc, Local, Timelike};
use prost::encoding::int32;
use reqwest;

use log::{debug, info, warn};

use axum::{
    extract::Path,
    extract::State, 
    extract::Query,
    response::Response,
    response::Json,
    routing::get,
    Router,
    middleware::map_response_with_state
};
use serde::{Serialize, Deserialize};
use serde_json::{json};

use tokio;
use tokio::time;
use tokio::sync::RwLock;

extern crate chrono;

pub mod transit_realtime {
     include!(concat!(env!("OUT_DIR"), "/transit_realtime.rs"));
}

pub mod mta {
    include!(concat!(env!("OUT_DIR"), "/stops.rs"));
}

#[derive(Serialize, Deserialize, Clone)]
struct Entry {
    stop: String,
    line: String,
    direction: String,
    leaving_at_timestamp: i64,
    arriving_at_timestamp: i64
}

#[derive(Default)]
struct NextTrainReq {
    from: String
}

#[derive(Deserialize, Default)]
struct NextTrainQuery {
    future_only: Option<bool>,
    limit_direction_departures: Option<i8>
}

#[derive(Default)]
struct MapByStop {
    rts_timestamp: DateTime<Utc>,
    sample_time: DateTime<Utc>,
    cache: HashMap<String, Vec<Entry>>
}

impl MapByStop {
    fn new(sample_time: DateTime<Utc>, rts_timestamp: DateTime<Utc>) -> Self {
        Self {
            rts_timestamp: rts_timestamp,
            sample_time: sample_time,
            cache: Default::default()
        }
    }

    fn append(&mut self, stop: String, entry: Entry) {
        let mut value = self.cache.entry(stop.to_owned()).or_default();
        // TODO: push sorted
        value.push(entry);
    }
}

struct AppState {
    cache_from: RwLock<MapByStop>
}

struct TransitRtResponse(DateTime<Utc>, Vec<Entry>);

#[derive(Debug)]
enum RequestError {
    ClientError(reqwest::Error),
    HttpError
}


fn handle_transit_rt(bytes: &[u8], filter_from: String) -> TransitRtResponse {
    let mut res = vec![];

    let msg  = transit_realtime::FeedMessage::decode(bytes).unwrap();

    let ts = Utc.timestamp(msg.header.timestamp.unwrap_or(0).try_into().unwrap(), 0);
    info!(
        "gtfs_realtime_version: {}, timestamp {}", 
        msg.header.gtfs_realtime_version, ts);

    for entity in msg.entity {
        if let Some(trip_update) = &entity.trip_update {
            let route_id = (&trip_update.trip).route_id.as_ref().map_or("Unlnown", |x| x.as_str());

            for stop in &trip_update.stop_time_update {
                let stop_id = stop.stop_id.as_ref().map(|x| x.as_str());
                let stop_name = stop_id.map_or(None, |x| mta::stop_name(x));

                if !filter_from.is_empty() && !stop_name.unwrap_or("").eq(&filter_from) {
                    continue;
                }

                let arrival: DateTime<Local> = Utc.timestamp(stop.arrival.as_ref().map(|a| a.time.unwrap_or(0)).unwrap_or(0), 0).into();
                let departure: DateTime<Local> = Utc.timestamp(stop.departure.as_ref().map(|a| a.time.unwrap_or(0)).unwrap_or(0), 0).into();

                
                debug!("Train: {}, stop: {} or {} at time: {}, delay: {}", 
                    route_id,
                    stop_id.unwrap_or("??"),
                    stop_name.unwrap_or("??"),
                    arrival,
                    stop.arrival.as_ref().map(|a| a.delay.unwrap_or(0)).unwrap_or(0)
                );

                let entry = Entry{
                    stop: stop_name.unwrap_or("??").to_owned(),
                    line: route_id.to_owned(),
                    direction: stop_id.unwrap_or("??").to_owned(),
                    arriving_at_timestamp: arrival.timestamp_millis() / 1000,
                    leaving_at_timestamp: departure.timestamp_millis() / 1000
                };
                
                res.push(entry);
            }
            
        }
    }

    TransitRtResponse{
        0: ts,
        1: res
    }
    
}

async fn next_train(req: NextTrainReq) -> Result<TransitRtResponse, RequestError> {
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
        .await.map_err(|err| RequestError::ClientError(err))?;
    

    if res.status() != 200 {
        warn!("next_train request failed with status={}", res.status());
        return Err(RequestError::HttpError);
    }

    let content = res.bytes().await.map_err(|err| RequestError::ClientError(err))?;
    Ok(handle_transit_rt(&content, req.from))
}

#[tokio::main]
async fn main() {
    env_logger::init();

    let state = Arc::new(
        AppState{
            cache_from: RwLock::new(Default::default())
        }
    );

    let mut interval = time::interval(Duration::from_millis(30000));

    loop {
        tokio::select! {
            _ = web_server(state.clone()) => {
            }

            _ = interval.tick() => {
                let res = next_train(NextTrainReq::default()).await;
                
                if let Err(err) = res {
                    match err {
                        RequestError::ClientError(err) => warn!("Got error: {}", err),
                        RequestError::HttpError => warn!("Got http error")
                    }
                    
                    continue;
                }

                let res = res.unwrap();

                let mut new_map = MapByStop::new(Utc::now(), res.0);
                for ent in res.1.into_iter() {
                    debug!("Got from: {}", ent.stop);
                    let stop = ent.stop.clone();
                    new_map.append(stop, ent);
                }

                let mut s = state.cache_from.write().await;
                _ = std::mem::replace(&mut *s, new_map);
                
            }
        }
    }
}

async fn next_train_handler(State(state): State<Arc<AppState>>, from: &str, future_only: Option<bool>, limit_direction_departures: Option<i8>) -> Json<Vec<Entry>> {
    let mut s = state.cache_from.read().await;
    let cache_from = &s.cache;
    let sample_time = s.sample_time.timestamp_millis() / 1000;

    let future_only = future_only.unwrap_or(false);
    let limit_direction_departures = limit_direction_departures.unwrap_or(0);

    let mut direction_counters: HashMap<&str, i8> = Default::default();


    match cache_from.get(from) {
        Some(entries) => Json(
            entries.iter().filter(|e| 
                !future_only || 
                (e.arriving_at_timestamp - sample_time) >= 0
            ).filter(|e| {
                *direction_counters.entry(e.direction.as_str()).or_default() += 1;
                limit_direction_departures <= 0 || direction_counters[e.direction.as_str()] <= limit_direction_departures
            })
            .map(|e| e.to_owned()).collect()

        ),
        None => Json(vec![])
    }
}

async fn all_next_train_handler(State(state): State<Arc<AppState>>) -> Json<serde_json::Value> {
    let mut s = state.cache_from.read().await;
    let cache_from = &s.cache;
    
    todo!("not implemented")
}

async fn stops(State(state): State<Arc<AppState>>) -> Json<Vec<String>> {
    let mut s = state.cache_from.read().await;
    let cache_from = &s.cache;

    let keys = cache_from.keys().map(|k| k.to_owned()).collect();
    Json(keys)

}

async fn set_header<A>(State(state): State<Arc<AppState>>, mut response: Response<A>) -> Response<A> {
    let timestamp_str = (Utc::now().timestamp_millis() / 1000).to_string();

    response.headers_mut().insert("x-timestamp", timestamp_str.parse().unwrap());
    response
}

async fn web_server(_state: Arc<AppState>) {
    // build our application with a single route
    let app = Router::new()
        .route("/", get(|| async { "Hello, World!" }))
        .route("/stops", get(
            stops
        ))
        .route("/next_train", get(
            all_next_train_handler
        ))
        .route("/next_train/from/:from", get(
            |Path(from): Path<String>, Query(q): Query<NextTrainQuery>, state: State<Arc<AppState>>| async move { 
                debug!("{}, {}", from, q.future_only.is_some());
                next_train_handler(
                    state, 
                    from.as_ref(), 
                    q.future_only,
                    q.limit_direction_departures
                ).await
            }
        ))
        .layer(map_response_with_state(_state.clone(), set_header))
        .with_state(_state)
    ;

    // run it with hyper on localhost:3000
    let svc = app.into_make_service();
    let addr = "0.0.0.0:3000".parse().unwrap();
    let srv = axum::Server::bind(&addr);
    
    srv.serve(svc).await.unwrap();
}
