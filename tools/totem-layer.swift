import AppKit
import CoreBluetooth
import IOKit.hid

final class LayerReader: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    private let serviceID = CBUUID(string: "EBB4F000-58AA-4562-A224-204BAD21C138")
    private let layerID = CBUUID(string: "EBB4F001-58AA-4562-A224-204BAD21C138")
    private let hidServiceID = CBUUID(string: "1812")
    private let notifications = DistributedNotificationCenter.default()
    private let update = Notification.Name("com.totem.layer.changed")
    private let request = Notification.Name("com.totem.layer.request")
    private let usb = IOHIDManagerCreate(kCFAllocatorDefault, 0)
    private var bluetooth: CBCentralManager!
    private var peripheral: CBPeripheral?
    private var usbDevice: IOHIDDevice?
    private var usbLayer: UInt8?
    private var bluetoothLayer: UInt8?
    private var publishedLayer: UInt8?

    func start() {
        notifications.addObserver(self, selector: #selector(publishCurrent), name: request, object: nil)
        let context = Unmanaged.passUnretained(self).toOpaque()
        IOHIDManagerSetDeviceMatching(usb, [
            kIOHIDDeviceUsagePageKey: 0xFF71,
            kIOHIDDeviceUsageKey: 1,
        ] as CFDictionary)
        IOHIDManagerRegisterDeviceMatchingCallback(usb, { context, _, _, device in
            guard let context else { return }
            Unmanaged<LayerReader>.fromOpaque(context).takeUnretainedValue().attachUSB(device)
        }, context)
        IOHIDManagerRegisterDeviceRemovalCallback(usb, { context, _, _, device in
            guard let context else { return }
            let reader = Unmanaged<LayerReader>.fromOpaque(context).takeUnretainedValue()
            guard reader.usbDevice == device else { return }
            reader.usbDevice = nil
            reader.usbLayer = nil
            reader.publish()
        }, context)
        IOHIDManagerRegisterInputReportCallback(usb, { context, result, sender, _, _, report, count in
            guard let context, let sender, result == kIOReturnSuccess, count == 1 else { return }
            let reader = Unmanaged<LayerReader>.fromOpaque(context).takeUnretainedValue()
            guard reader.usbDevice.map({ Unmanaged.passUnretained($0).toOpaque() == sender }) == true else { return }
            reader.setUSBLayer(report[0])
        }, context)
        IOHIDManagerScheduleWithRunLoop(usb, CFRunLoopGetMain(), CFRunLoopMode.defaultMode.rawValue)
        let result = IOHIDManagerOpen(usb, 0)
        if result != kIOReturnSuccess { NSLog("Totem USB HID open failed: %d", result) }

        bluetooth = CBCentralManager(delegate: self, queue: nil,
                                     options: [CBCentralManagerOptionShowPowerAlertKey: false])
        Timer.scheduledTimer(withTimeInterval: 2, repeats: true) { [weak self] _ in
            self?.findBluetooth()
        }
    }

    private func attachUSB(_ device: IOHIDDevice) {
        guard usbDevice == nil else { return }
        usbDevice = device
        var value: UInt8 = 0
        var size = 1
        if IOHIDDeviceGetReport(device, kIOHIDReportTypeInput, 0, &value, &size) == kIOReturnSuccess,
           size == 1 {
            setUSBLayer(value)
        }
    }

    private func setUSBLayer(_ value: UInt8) {
        guard value < 7 else { return }
        usbLayer = value
        publish()
    }

    private func findBluetooth() {
        guard bluetooth.state == .poweredOn, peripheral == nil else { return }
        let devices = bluetooth.retrieveConnectedPeripherals(withServices: [serviceID, hidServiceID])
            .filter { $0.name?.localizedCaseInsensitiveContains("totem") == true }
        guard devices.count == 1, let device = devices.first else { return }
        peripheral = device
        device.delegate = self
        bluetooth.connect(device)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            findBluetooth()
        } else {
            clearBluetooth()
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.discoverServices([serviceID])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        if self.peripheral == peripheral { clearBluetooth() }
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                        error: Error?) {
        if self.peripheral == peripheral { clearBluetooth() }
    }

    private func clearBluetooth() {
        peripheral = nil
        bluetoothLayer = nil
        publish()
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceID }) else { return }
        peripheral.discoverCharacteristics([layerID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        guard let layer = service.characteristics?.first(where: { $0.uuid == layerID }) else { return }
        peripheral.setNotifyValue(true, for: layer)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic,
                    error: Error?) {
        if characteristic.isNotifying { peripheral.readValue(for: characteristic) }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        guard self.peripheral == peripheral, characteristic.uuid == layerID, error == nil,
              let data = characteristic.value, data.count == 1, let layer = data.first, layer < 7 else { return }
        bluetoothLayer = layer
        publish()
    }

    private func publish() {
        let layer = usbLayer ?? bluetoothLayer
        guard layer != publishedLayer else { return }
        publishedLayer = layer
        publishCurrent()
    }

    @objc private func publishCurrent() {
        notifications.postNotificationName(update, object: nil,
                                           userInfo: ["layer": publishedLayer.map(Int.init) ?? -1],
                                           deliverImmediately: true)
    }
}

let reader = LayerReader()
reader.start()
RunLoop.main.run()
