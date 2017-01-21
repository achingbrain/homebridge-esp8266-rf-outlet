'use strict'

const EventEmitter = require('events').EventEmitter
const ssdp = require('@achingbrain/ssdp')

const USN = 'urn:schemas-upnp-org:device:ESP8266RFOutlet'
const emitter = new EventEmitter()

const toOutlet = (service) => {
  return {
    id: service.UDN,
    url: service.details.URLBase,
    name: service.details.device.friendlyName,
    maunfacturer: service.details.device.manufacturer,
    model: service.details.device.modelName,
    serialNumber: service.details.device.serialNumber,
    firmwareRevision: 1
  }
}

const bus = ssdp()
bus.on('error', emitter.emit.bind(emitter, 'error'))

bus.on('discover:' + USN, (service) => {
  emitter.emit('outlet:found', toOutlet(service))

  bus.on('update:' + service.UDN, (service) => {
    emitter.emit('outlet:update', toOutlet(service))
  })

  bus.on('remove:' + service.UDN, (service) => {
    emitter.emit('outlet:lost', toOutlet(service))
  })
})

process.on('SIGINT', () => {
  setTimeout(process.exit.bind(null, 1), 1000)

  bus.stop(function (error) {
    process.exit(error ? 1 : 0)
  })
})

emitter.discover = () => {
  bus.discover(USN)
}

module.exports = emitter
