/**
 *  Honeywell Contact Sensor
 *
 *  Copyright 2015 Justin Haines
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */
metadata {
	definition (name: "Honeywell Contact Sensor", namespace: "jhaines0", author: "Justin Haines") {
		capability "Actuator"
		capability "Battery"
		capability "Contact Sensor"
		capability "Refresh"
		capability "Sensor"

		attribute "tamper", "string"
	}

	simulator {
		// TODO: define status and reply messages here
	}

	tiles(scale: 2) {
        standardTile("contact", "device.contact", width: 6, height: 4) {
            state("open", label:'open', icon:"st.contact.contact.open", backgroundColor:"#ffa81e")
            state("closed", label:'closed', icon:"st.contact.contact.closed", backgroundColor:"#79b821")
        }
        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
            state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
        }
        standardTile("tamper", "device.tamper", width: 2, height: 2) {
            state("true", label:'tamper detected', icon:"st.categories.damageAndDanger", backgroundColor:"#ffa81e")
            state("false", label:'no tamper', icon:"st.nest.empty", backgroundColor:"#79b821")
        }
        main(["contact"])
        details(["contact","refresh","tamper"])
	}
}

// parse events into attributes
def parse(String description) {
}

def update(Map data) {
	sendEvent(name: "contact", value: (data.alarm ? "open" : "closed"))
    sendEvent(name: "battery", value: (data.lowBattery ? 10 : 100))
    sendEvent(name: "tamper", value: data.tamper)
}

// handle commands
def refresh() {
	parent.requestUpdate()
}


