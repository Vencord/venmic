type DefaultProps = 'node.name' | 'application.name';

type LiteralUnion<
	LiteralType,
	BaseType extends string,
> = LiteralType | (BaseType & Record<never, never>);

export interface Prop
{
    key: string;
    value: string;
}

export class PatchBay
{
    unlink(): void;
    
    list<T extends string = DefaultProps>(props?: T[]): Record<LiteralUnion<T, string>, string>[];
    link(data: {props: Prop[], mode: "include" | "exclude"}): boolean;

    static hasPipeWire(): boolean;
}
