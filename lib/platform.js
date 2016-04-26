'use strict'

const pkg = require('../package.json')
const discovery = require('./discovery')
const homebridge = require('./homebridge')
const Outlet = require('./outlet')
const outlets = {}

module.exports = (server) => {
  homebridge.Service = server.hap.Service
  homebridge.Characteristic = server.hap.Characteristic

  server.registerPlatform('homebridge-esp8266-rf-outlet', 'esp8266-rf-outlet', ESP8266RFPlatform)
}

class ESP8266RFPlatform {
  constructor (log, config) {
    this.log = log
    this.search = config['search'] || 5000

    this.log(`ESP8266 RF Outlet Platform Plugin Version ${this.getVersion()}`)

    discovery.on('error', (error) => {
      console.error(error)
    })

    discovery.on('outlet:found', (outlet) => {
      outlets[outlet.id] = new Outlet(log, outlet)
    })

    discovery.on('outlet:update', (outlet) => {
      outlets[outlet.id] = new Outlet(log, outlet)
    })

    discovery.on('outlet:lost', (rfSwitch) => {
      delete outlets[rfSwitch.id]
    })
  }

  accessories (callback) {
    this.log('Searching for ESP8266 microcontrollers')

    discovery.discover()

    // wait for switches to respond
    setTimeout(() => {
      this.log(`Asked for accessories, have ${Object.keys(outlets).length} outlets after ${this.search}ms`)

      callback(Object.keys(outlets).map((key) => {
        return outlets[key]
      }))
    }, this.search)
  }

  getVersion () {
    return pkg.version
  }
}
