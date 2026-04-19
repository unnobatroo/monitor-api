from fastapi import FastAPI
from app.db.session import Base, engine
from app.routes import node, reading

app = FastAPI(title="grid monitor API")

app.include_router(node.router, prefix="/nodes", tags=["Nodes"])
app.include_router(reading.router, prefix="/readings", tags=["Readings"])

Base.metadata.create_all(bind=engine)

@app.get("/")
def read_root():
    return {"message": "online!"}