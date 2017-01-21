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
      console.error(error.stack ? error.stack : error.toString())
    })

    discovery.on('outlet:found', (outlet) => {
      this.log('Found outlet', outlet.name, outlet.id)
      outlets[outlet.id] = new Outlet(log, outlet)
    })

    discovery.on('outlet:update', (outlet) => {
      this.log('Outlet', outlet.name, outlet.id, 'was updated')
      outlets[outlet.id] = new Outlet(log, outlet)
    })

    discovery.on('outlet:lost', (outlet) => {
      this.log('Outlet', outlet.name, outlet.id, 'went away')
      delete outlets[outlet.id]
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
