'use strict'

const EventEmitter = require('events').EventEmitter
const request = require('request')
const homebridge = require('./homebridge')

const STATES = {
  ON: 'ON',
  OFF: 'OFF',
  IDENTIFY: 'IDENTIFY'
}

class Outlet extends EventEmitter {
  constructor (log, config) {
    super()

    this.log = log
    this.id = config.id
    this.url = config.url
    this.state = STATES.OFF

    this.name = 'Outlet #' + this.id
    this.uuid_base = this.id

    this.update()
  }

  _identify () {
    if (this.state !== STATES.IDENTIFY) {
      return
    }

    setTimeout(this._identify.bind(this), 1000)
  }

  update (on, callback) {
    if (callback) {
      this.once('update', callback)
    }

    if (this._debounceRequest) {
      clearTimeout(this._debounceRequest)
    }

    const outlet = this
    on = !!on

    this._debounceRequest = setTimeout(() => {
      let url = outlet.url

      if (on) {
        url += 'light/on'
      } else {
        url += 'light/off'
      }

      outlet.log(`Setting switch to ${on} with url ${url}`)

      request.get(url, function (error, response) {
        if (error) {
          console.error(error)
        }

        if (!error) {
          outlet.state = on ? STATES.ON : STATES.OFF
        }

        outlet.emit('update', error)
      })
    })
  }

  getState (characteristic, callback) {
    switch (characteristic.toLowerCase()) {
      case 'state':
        return callback(null, this.state === STATES.ON)
    }

    callback(new Error('Unknown characteristic ' + characteristic))
  }

  identify (callback) {
    callback()
  }

  getServices () {
    const outlet = this
    const service = new homebridge.Service.Outlet(this.name, this.id)

    service
      .getCharacteristic(homebridge.Characteristic.On)
      .on('get', (callback) => {
        callback(null, outlet.state === STATES.ON)
      })
      .on('set', (value, callback) => {
        outlet.log(`Updating on to ${value}`)
        outlet.update(value, callback)
      })
      .value = outlet.state === STATES.ON

    const informationService = new homebridge.Service.AccessoryInformation()

    informationService
    .setCharacteristic(homebridge.Characteristic.Manufacturer, 'AchingBrain')
    .setCharacteristic(homebridge.Characteristic.Model, 'ESP8266RFO001')
    .setCharacteristic(homebridge.Characteristic.SerialNumber, this.id)
    .addCharacteristic(homebridge.Characteristic.FirmwareRevision, 1)

    return [informationService, service]
  }
}

module.exports = Outlet
