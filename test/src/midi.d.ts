/**
 * Type definitions for the 'midi' Node.js package
 */

declare module 'midi' {
  export class Input {
    constructor();
    getPortCount(): number;
    getPortName(port: number): string;
    openPort(port: number): void;
    closePort(): void;
    isPortOpen(): boolean;
    on(event: 'message', callback: (deltaTime: number, message: number[]) => void): this;
  }

  export class Output {
    constructor();
    getPortCount(): number;
    getPortName(port: number): string;
    openPort(port: number): void;
    closePort(): void;
    isPortOpen(): boolean;
    sendMessage(message: number[]): void;
  }
}