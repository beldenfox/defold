package com.dynamo.cr.tileeditor.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.Set;

import org.junit.Before;
import org.junit.Test;

import com.dynamo.cr.tileeditor.core.IMapView;
import com.dynamo.cr.tileeditor.core.MapModel;
import com.dynamo.cr.tileeditor.core.MapPresenter;
import com.dynamo.tile.proto.Tile.TileMap;

public class MapTest {
    IMapView view;
    MapModel model;
    MapPresenter presenter;

    @Before
    public void setup() {
        this.view = mock(IMapView.class);
        this.model = new MapModel();
        this.presenter = new MapPresenter(this.model, this.view);
    }

    /**
     * Use Case 1.1
     */
    @Test
    public void testSuperMarioMap() {
        // new file
        TileMap tileMap = TileMap.newBuilder()
                .setImage("")
                .setTileWidth(0)
                .setTileHeight(0)
                .setTileCount(0)
                .setTileSpacing(0)
                .setTileMargin(0)
                .setCollision("")
                .setMaterialTag("tile")
                .build();
        this.presenter.load(tileMap);

        verify(this.view).setImage(eq(""));
        assertEquals("", this.model.getImage());
        verify(this.view).setTileWidth(eq(0));
        assertEquals(0, this.model.getTileWidth());
        verify(this.view).setTileHeight(eq(0));
        assertEquals(0, this.model.getTileHeight());
        verify(this.view).setTileCount(eq(0));
        assertEquals(0, this.model.getTileCount());
        verify(this.view).setTileMargin(eq(0));
        assertEquals(0, this.model.getTileMargin());
        verify(this.view).setTileSpacing(eq(0));
        assertEquals(0, this.model.getTileSpacing());
        verify(this.view).setCollision(eq(""));
        assertEquals("", this.model.getCollision());
        verify(this.view).setMaterialTag(eq("tile"));
        assertEquals("tile", this.model.getMaterialTag());

        // set properties
        this.presenter.setImage("my_mario_map.png");
        assertEquals("my_mario_map.png", this.model.getImage());
        this.presenter.setTileWidth(16);
        assertEquals(16, this.model.getTileWidth());
        this.presenter.setTileHeight(32);
        assertEquals(32, this.model.getTileHeight());
        this.presenter.setTileCount(4);
        assertEquals(4, this.model.getTileCount());
        this.presenter.setTileMargin(1);
        assertEquals(1, this.model.getTileMargin());
        this.presenter.setTileSpacing(2);
        assertEquals(2, this.model.getTileSpacing());

        // select tiles

        assertTrue(this.model.getSelectedTiles().isEmpty());

        //   select tile outside the map
        this.presenter.selectTile(0, 2);
        assertTrue(this.model.getSelectedTiles().isEmpty());
        verify(this.view, never()).setSelectedTiles(any(Set.class));

        //   select obstruction tile
        this.presenter.selectTile(0, 1);
        assertEquals(1, this.model.getSelectedTiles().size());
        verify(this.view, times(1)).setSelectedTiles(any(Set.class));

        this.presenter.setTileCollisionGroup("obstruction");
        for (MapModel.Tile tile : this.model.getSelectedTiles()) {
            assertEquals("obstruction", tile.getCollisionGroup());
        }

        //   select hazard
        this.presenter.selectTile(1, 0);
        assertEquals(1, this.model.getSelectedTiles().size());
        verify(this.view, times(2)).setSelectedTiles(any(Set.class));

        this.presenter.setTileCollisionGroup("hazard");
        for (MapModel.Tile tile : this.model.getSelectedTiles()) {
            assertEquals("hazard", tile.getCollisionGroup());
        }

    }
}
