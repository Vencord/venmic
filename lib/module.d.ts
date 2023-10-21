type DefaultProps = 'node.name' | 'application.name';

export class PatchBay
{
    unlink(): void;
    
    list<T extends string = DefaultProps>(props?: T[]): Record<T, string>[];
    link(data: {key: string, value: string, mode: "include" | "exclude"}): boolean;

    static hasPipeWire(): boolean;
}
