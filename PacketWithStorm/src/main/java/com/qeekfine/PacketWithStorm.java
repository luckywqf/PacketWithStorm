/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.qeekfine;

import backtype.storm.Config;
import backtype.storm.LocalCluster;
import backtype.storm.StormSubmitter;
import backtype.storm.spout.ShellSpout;
import backtype.storm.topology.*;
import backtype.storm.topology.base.BaseBasicBolt;
import backtype.storm.tuple.Fields;
import backtype.storm.tuple.Tuple;
import backtype.storm.tuple.Values;

import java.util.HashMap;
import java.util.Map;

import org.json.simple.*;
import org.json.simple.parser.*;

/**
 * This topology demonstrates Storm's stream groupings and multilang capabilities.
 */
public class PacketWithStorm {
	public static class PacketSpout extends ShellSpout implements IRichSpout {

		public PacketSpout() {
			super("bash", "start_app.sh");
		}

		public void declareOutputFields(OutputFieldsDeclarer declarer) {
			declarer.declare(new Fields("word"));
		}

		public Map<String, Object> getComponentConfiguration() {
			return null;
		}
	}

	public static class PacketGroup extends BaseBasicBolt {

		public void execute(Tuple tuple, BasicOutputCollector collector) {
			String str = tuple.getString(0);			
			JSONParser parser = new JSONParser();
			JSONObject root;
			try {
				root = (JSONObject)parser.parse(str);
			}
			catch(ParseException pe) {
				System.out.println("position: " + pe.getPosition());
				System.out.println(pe);
				return;
			}
			String hdsrc = (String)root.get("ether_src");

			collector.emit(new Values(hdsrc, root));
		}

		public void declareOutputFields(OutputFieldsDeclarer declarer) {
			declarer.declare(new Fields("src", "root"));
		}
	}


	public static class PacketSendCount extends BaseBasicBolt {
		Map<String, Integer> counts = new HashMap<String, Integer>();

		public void execute(Tuple tuple, BasicOutputCollector collector) {
			String src = tuple.getString(0);
			Integer count = counts.get(src);
			if (count == null)
				count = 0;
			count++;
			counts.put(src, count);
			collector.emit(new Values(src, count));
		}

		public void declareOutputFields(OutputFieldsDeclarer declarer) {
			declarer.declare(new Fields("src", "count"));
		}
	}
  
  
	public static void main(String[] args) throws Exception {

		TopologyBuilder builder = new TopologyBuilder();

		builder.setSpout("spout", new PacketSpout(), 5);

		builder.setBolt("group", new PacketGroup(), 8).shuffleGrouping("spout");
		builder.setBolt("count", new PacketSendCount(), 12).fieldsGrouping("group", new Fields("src"));

		Config conf = new Config();
		conf.setDebug(true);


		if (args != null && args.length > 0) {
			conf.setNumWorkers(3);

			StormSubmitter.submitTopologyWithProgressBar(args[0], conf, builder.createTopology());
		}
		else {
			conf.setMaxTaskParallelism(3);

			LocalCluster cluster = new LocalCluster();
			cluster.submitTopology("word-count", conf, builder.createTopology());

			Thread.sleep(10000);

			cluster.shutdown();
		}
	}
}
