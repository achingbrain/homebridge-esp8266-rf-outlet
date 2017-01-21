'use strict'

const EventEmitter = require('events').EventEmitter
const request = require('request')
const homebridge = require('./homebridge')

class Outlet extends EventEmitter {
  constructor (log, config) {
    super()

    this.log = log
    this.id = config.id
    this.url = config.url
    this.maunfacturer = config.manufacturer
    this.model = config.model
    this.serialNumber = config.serialNumber
    this.firmwareRevision = config.firmwareRevision
    this.name = config.name
    this.uuid_base = this.id
  }

  update (on, callback) {
    if (callback) {
      this.once('update', callback)
    }

    if (this._debounceRequest) {
      clearTimeout(this._debounceRequest)
    }

    const state = on ? 'ON' : 'OFF'
    const url = this.url + 'outlet'

    this._debounceRequest = setTimeout(() => {
      this.log(`Setting outlet ${this.name} state to ${state} with url ${url}`)

      request.patch({
        url: url,
        json: true,
        body: {
          status: state
        }
      }, (error, response) => {
        this.log(`Set outlet ${this.name} state to ${state} with url ${url}`)
        this.emit('update', error)
      })
    })
  }

  getState (characteristic, callback) {
    if (characteristic.toLowerCase() === 'state') {
      const url = this.url + 'outlet'

      this.log(`Getting outlet ${this.name} state with url ${url}`)

      return request.get(url, (error, response, body) => {
        if (error) {
          return callback(error)
        }

        try {
          var result = JSON.parse(body)
          this.log(`Got outlet ${this.name} state ${result.state} from url ${url}`)

          callback(null, result.state.toLowerCase() === 'on')
        } catch (e) {
          callback(e)
        }
      })
    }

    callback(new Error('Unknown characteristic ' + characteristic))
  }

  identify (callback) {
    this.log('Identifying', this.name, 'with url', this.url + 'identify')
    request.post({
      url: this.url + 'identify',
      json: true,
      body: {}
    }, callback)
  }

  getServices () {
    const service = new homebridge.Service.Outlet(this.name, this.id)

    service
      .getCharacteristic(homebridge.Characteristic.On)
      .on('get', this.getState.bind(this, 'state'))
      .on('set', this.update.bind(this))

    const informationService = new homebridge.Service.AccessoryInformation()

    informationService
      .setCharacteristic(homebridge.Characteristic.Manufacturer, this.manufacturer)
      .setCharacteristic(homebridge.Characteristic.Model, this.model)
      .setCharacteristic(homebridge.Characteristic.SerialNumber, this.serialNumber)
      .addCharacteristic(homebridge.Characteristic.FirmwareRevision, this.firmwareRevision)

    return [informationService, service]
  }
}

module.exports = Outlet
