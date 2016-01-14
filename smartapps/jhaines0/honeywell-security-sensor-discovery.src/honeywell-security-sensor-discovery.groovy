/**
 *  Honeywell Security Sensor Discovery
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
definition(
    name: "Honeywell Security Sensor Discovery",
    namespace: "jhaines0",
    author: "Justin Haines",
    description: "Service Manager to discover Honeywell security devices made available by a server on the LAN",
    category: "Safety & Security",
    iconUrl: "https://s3.amazonaws.com/smartapp-icons/SafetyAndSecurity/Cat-SafetyAndSecurity.png",
    iconX2Url: "https://s3.amazonaws.com/smartapp-icons/SafetyAndSecurity/Cat-SafetyAndSecurity@2x.png",
    iconX3Url: "https://s3.amazonaws.com/smartapp-icons/SafetyAndSecurity/Cat-SafetyAndSecurity@3x.png")


preferences {
	input name: "uri", type: "text", title: "Enter Server URI", required: false
    input(name: "serialInput", type: "text", title: "Desired Serial Numbers", multiple: true, required: false)
}

mappings {
  path("/event") {
    action: [
      PUT: "eventOccurred"
    ]
  }
}

def installed() {
	parseSerialList()
    initialize()
}

def uninstalled() {
    removeChildDevices(getChildDevices())
}

private removeChildDevices(delete) {
    delete.each {
        deleteChildDevice(it.deviceNetworkId)
    }
}

def updated() {
	parseSerialList()
}

def parseSerialList() {
	try
    {
		state.serials = []
		"${serialInput}".tokenize('[],').each { state.serials << it.toInteger() }
    }
    catch(e)
    {
    	log.error "Error parsing device list: ${e}"
    }
}

def initialize() {
    subscribe(location, null, locationHandler, [filterEvents:false])
	requestUpdate()
}

def handleIncomingData(json) {
    json.each {
        if(state.serials.contains(it.serial))
        {
        	def dni = "${it.serial}"

            try {
                def existingDevice = getChildDevice(dni)

                if(!existingDevice)
                {
                    def typeName;
                    if(it.isMotion)
                    {
                        typeName = "Honeywell Motion Sensor"
                    }
                    else
                    {
                        typeName = "Honeywell Contact Sensor"
                    }
                    existingDevice = addChildDevice("jhaines0", typeName, dni, null, [name: "${dni}", label: "${it.serial}"])
                }

                existingDevice.update(it)

            } catch (e) {
                log.error "Error creating device: ${e}"
            }
        }
    }
}

def locationHandler(evt) {
    def msg = parseLanMessage(evt.description)
    
    if(msg.json)
    {
		handleIncomingData(msg.json)
    }
}


def requestUpdate() {
	def theAction = new physicalgraph.device.HubAction(
		method: "GET",
		path: "/deviceState.json",
		headers: [
			HOST: "${uri}:80",
		])
     
    sendHubCommand(theAction)
}


def eventOccurred() {
    def json = request.JSON
    if(json)
    {
    	handleIncomingData(json)
    }
}