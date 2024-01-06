type DefaultProps = 'node.name' | 'application.name';

type LiteralUnion<
	LiteralType,
	BaseType extends string,
> = LiteralType | (BaseType & Record<never, never>);

type Optional<
    Type, 
    Key extends keyof Type
> = Partial<Pick<Type, Key>> & Omit<Type, Key>;

export interface Prop
{
    key: string;
    value: string;
}

export interface LinkData
{
    include: Prop[];
    exclude: Prop[];

    ignore_devices?: boolean;
}


export class PatchBay
{
    unlink(): void;
    
    list<T extends string = DefaultProps>(props?: T[]): Record<LiteralUnion<T, string>, string>[];
    link(data: Optional<LinkData, "exclude"> | Optional<LinkData, "include">): boolean;

    static hasPipeWire(): boolean;
}
