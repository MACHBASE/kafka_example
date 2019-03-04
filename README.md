# Machbase Kafka example

This is source code for tutorial of connecting kafka's streaming event to Machbase. It is based on [edenhill/librdkafka](https://github.com/edenhill/librdkafka) library, including Machbase's C connector. You can freely use with or without tutorial page at [link here](#).

### Preparation

#### Setting environment variable
```bash
export MACHBASE_IP='localhost'
export MACHBASE_PORT_NO=5656
```

If you don't set those values, then default IP/Port will be affected. (127.0.0.1/5656)

#### Install & startup Machbase
Please follow install instructions via [documentation](https://doc.machbase.com/).
(If you want to read Korean manual, please follow [here](http://krdoc.machbase.com).)

#### Install & startup Apache Kafka
Please follow [Kafka's quickstart page](http://kafka.apache.org/quickstart).
Then, remember Kafka installation path.

### Test
#### Compiling
After cloning this repo, go inside and type;

```bash
make all
```

#### CREATE TAGDATA TABLE
Using machsql, run the `CREATE TABLE` query writter in `query/create.sql`.

At repository directory,
```bash
machsql -f query/create.sql
```

#### Generating tag.csv
At repository directory,
```bash
python gen.py ${COUNT} > /path/to/kafka/install/tag.csv
```

`${COUNT}` is the number of tag data that you want to populate.

#### Generating topic (tag)
At kafka's directory, 
```bash
$ bin/kafka-topics.sh --create --zookeeper localhost:2181 --replication-factor 1 --partitions 1 --topic tag
```

If you install and startup zookeeper server at other IP/Port, you should change connection information described above.

#### Consumer
At repository directory,
```bash
./kafka_to_machbase tag
```

It waits further data to consume from the kafka..

#### Producer
At kafka's directory, 
```bash
bin/kafka-console-producer.sh --broker-list localhost:9092 --topic tag < tag.csv
```

If you install and startup kafka broker at other IP/Port, you should change connection information described above.

It produces tag data written in tag.csv, and the consumer will retrieve and append to Machbase server.

#### Checking
Using machsql, run `SELECT` query to the tagdata table.

```sql
SELECT COUNT(*) FROM TAG;
```
